# To debug kernel startup silent stuck:
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
# boot with NFS filesystem mount
```sh
STM32MP> setenv bootargs 'console=ttySTM0,115200 root=/dev/nfs nfsroot=192.168.1.43:/srv/nfs/rootfs ip=dhcp'
STM32MP> print bootcmd
bootcmd=dhcp;tftp c2000000 uImage;tftp c4000000 stm32mp157d-andy.dtb;bootm c2000000 - c4000000
```
