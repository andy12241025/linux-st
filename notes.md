To debug startup silent stuck:
- Rebuild once with low-level debugging enabled:
```sh
  make menuconfig
  Enable:
  Kernel hacking
    └─ Kernel low-level debugging functions
         └─ Use STM32MP1 UART for low-level debug
    └─ Early printk
```

- Then rebuild and boot with earlyprintk added to bootargs:
```sh
  setenv bootargs 'console=ttySTM0,115200 earlyprintk loglevel=8 ...'
```

