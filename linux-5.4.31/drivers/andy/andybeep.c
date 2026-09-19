#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>

#define BEEP_CNT		1
#define BEEP_NAME		"andybeep"
#define BEEPOFF			0
#define BEEPON			1

struct beep_dev {
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	int beep_gpio;
};

static struct beep_dev beep;

/*
 * PC7 feeds the base of the S8550 PNP transistor, so a low level turns the
 * buzzer on. The device tree flags the GPIO active low, hence the gpiod-style
 * helper below takes the logical level and gpio_set_value() gets the raw one.
 */
static void beep_switch(u8 sta)
{
	gpio_set_value(beep.beep_gpio, (sta == BEEPON) ? 0 : 1);
}

static int beep_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &beep;
	return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	unsigned char databuf[1];

	if (cnt != sizeof(databuf))
		return -EINVAL;

	if (copy_from_user(databuf, buf, cnt)) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	if (databuf[0] == BEEPON)
		beep_switch(BEEPON);
	else if (databuf[0] == BEEPOFF)
		beep_switch(BEEPOFF);
	else
		return -EINVAL;

	return cnt;
}

static int beep_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations beep_fops = {
	.owner = THIS_MODULE,
	.open = beep_open,
	.write = beep_write,
	.release = beep_release,
};

static int __init beep_init(void)
{
	int ret;

	beep.nd = of_find_node_by_path("/andybeep");
	if (beep.nd == NULL) {
		printk("No andybeep node found!\r\n");
		return -EINVAL;
	}

	beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpio", 0);
	if (!gpio_is_valid(beep.beep_gpio)) {
		printk("beep-gpio property read failed!\r\n");
		return -EINVAL;
	}
	printk("beep-gpio num = %d\r\n", beep.beep_gpio);

	ret = gpio_request(beep.beep_gpio, "beep");
	if (ret) {
		pr_err("%s: failed to request gpio %d\r\n", BEEP_NAME, beep.beep_gpio);
		return ret;
	}

	/* start with the buzzer silent (raw high keeps Q1 off) */
	ret = gpio_direction_output(beep.beep_gpio, 1);
	if (ret < 0)
		goto free_gpio;

	if (beep.major) {
		beep.devid = MKDEV(beep.major, 0);
		ret = register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
	} else {
		ret = alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
		beep.major = MAJOR(beep.devid);
		beep.minor = MINOR(beep.devid);
	}
	if (ret < 0) {
		pr_err("%s: couldn't get a devid, ret=%d\r\n", BEEP_NAME, ret);
		goto free_gpio;
	}
	printk("andybeep major=%d,minor=%d\r\n", beep.major, beep.minor);

	beep.cdev.owner = THIS_MODULE;
	cdev_init(&beep.cdev, &beep_fops);
	ret = cdev_add(&beep.cdev, beep.devid, BEEP_CNT);
	if (ret < 0)
		goto del_unregister;

	beep.class = class_create(THIS_MODULE, BEEP_NAME);
	if (IS_ERR(beep.class)) {
		ret = PTR_ERR(beep.class);
		goto del_cdev;
	}

	beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
	if (IS_ERR(beep.device)) {
		ret = PTR_ERR(beep.device);
		goto destroy_class;
	}

	return 0;

destroy_class:
	class_destroy(beep.class);
del_cdev:
	cdev_del(&beep.cdev);
del_unregister:
	unregister_chrdev_region(beep.devid, BEEP_CNT);
free_gpio:
	gpio_free(beep.beep_gpio);
	return ret;
}

static void __exit beep_exit(void)
{
	beep_switch(BEEPOFF);

	device_destroy(beep.class, beep.devid);
	class_destroy(beep.class);
	cdev_del(&beep.cdev);
	unregister_chrdev_region(beep.devid, BEEP_CNT);
	gpio_free(beep.beep_gpio);
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDY");
MODULE_INFO(intree, "Y");
