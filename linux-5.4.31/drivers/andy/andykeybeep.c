// SPDX-License-Identifier: GPL-2.0
/*
 * KEY1 (PH7) controls the buzzer (PC7): the beep sounds while KEY1 is held
 * and stops when it is released.
 *
 * KEY1 shorts PH7 to GND when pressed and its external pull-up (R15) is not
 * fitted, so the internal pull-up is enabled here. Both GPIOs are flagged
 * active low in the device tree, so logical 1 means "pressed" / "beep on".
 *
 * Top half: the key IRQ fires on both edges and only (re)arms a delayed
 * work. Bottom half: the work runs once the line has been quiet for the
 * debounce interval, samples the key and copies its state to the buzzer.
 * Since each edge pushes the work out again, bounces are absorbed and the
 * final level is never missed.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>

#define KEYBEEP_NAME		"andykeybeep"
#define KEYBEEP_DEBOUNCE_MS	10

struct keybeep_dev {
	struct device *dev;
	struct gpio_desc *key_gpio;
	struct gpio_desc *beep_gpio;
	int irq;
	unsigned int debounce_ms;
	struct delayed_work work;
};

static void keybeep_work_func(struct work_struct *work)
{
	struct keybeep_dev *kb = container_of(to_delayed_work(work),
					      struct keybeep_dev, work);
	int pressed;

	pressed = gpiod_get_value_cansleep(kb->key_gpio);
	if (pressed < 0)
		return;

	gpiod_set_value_cansleep(kb->beep_gpio, pressed);
	dev_dbg(kb->dev, "KEY1 %s\n", pressed ? "pressed" : "released");
}

static irqreturn_t keybeep_isr(int irq, void *dev_id)
{
	struct keybeep_dev *kb = dev_id;

	mod_delayed_work(system_wq, &kb->work,
			 msecs_to_jiffies(kb->debounce_ms));
	return IRQ_HANDLED;
}

static void keybeep_cancel_work(void *data)
{
	struct keybeep_dev *kb = data;

	cancel_delayed_work_sync(&kb->work);
	gpiod_set_value_cansleep(kb->beep_gpio, 0);
}

static int keybeep_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct keybeep_dev *kb;
	int ret;

	kb = devm_kzalloc(dev, sizeof(*kb), GFP_KERNEL);
	if (!kb)
		return -ENOMEM;
	kb->dev = dev;

	if (of_property_read_u32(dev->of_node, "debounce-interval",
				 &kb->debounce_ms))
		kb->debounce_ms = KEYBEEP_DEBOUNCE_MS;

	/* start with the buzzer silent */
	kb->beep_gpio = devm_gpiod_get(dev, "beep", GPIOD_OUT_LOW);
	if (IS_ERR(kb->beep_gpio)) {
		ret = PTR_ERR(kb->beep_gpio);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get beep-gpios: %d\n", ret);
		return ret;
	}

	kb->key_gpio = devm_gpiod_get(dev, "key", GPIOD_IN);
	if (IS_ERR(kb->key_gpio)) {
		ret = PTR_ERR(kb->key_gpio);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get key-gpios: %d\n", ret);
		return ret;
	}

	/*
	 * 5.4 gpiolib ignores the GPIO_PULL_UP DT flag, so ask the pin
	 * controller for the pull-up explicitly (R15 is not populated).
	 */
	ret = pinctrl_gpio_set_config(desc_to_gpio(kb->key_gpio),
				      pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 1));
	if (ret)
		dev_warn(dev, "failed to enable key pull-up: %d\n", ret);

	kb->irq = gpiod_to_irq(kb->key_gpio);
	if (kb->irq < 0) {
		dev_err(dev, "failed to get key irq: %d\n", kb->irq);
		return kb->irq;
	}

	INIT_DELAYED_WORK(&kb->work, keybeep_work_func);
	/* registered before the IRQ, so devm runs it after free_irq() */
	ret = devm_add_action_or_reset(dev, keybeep_cancel_work, kb);
	if (ret)
		return ret;

	ret = devm_request_irq(dev, kb->irq, keybeep_isr,
			       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			       KEYBEEP_NAME, kb);
	if (ret) {
		dev_err(dev, "failed to request irq %d: %d\n", kb->irq, ret);
		return ret;
	}

	/* sync with the key in case it is already held at load time */
	mod_delayed_work(system_wq, &kb->work, 0);

	platform_set_drvdata(pdev, kb);
	dev_info(dev, "KEY1 irq %d, debounce %u ms\n", kb->irq, kb->debounce_ms);
	return 0;
}

static const struct of_device_id keybeep_of_match[] = {
	{ .compatible = "andy,key-beep" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, keybeep_of_match);

static struct platform_driver keybeep_driver = {
	.probe = keybeep_probe,
	.driver = {
		.name = KEYBEEP_NAME,
		.of_match_table = keybeep_of_match,
	},
};
module_platform_driver(keybeep_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDY");
MODULE_DESCRIPTION("KEY1 press-and-hold beep driver");
MODULE_INFO(intree, "Y");
