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
# make boot ext4 
```sh
  dd if=/dev/zero of=bootfs.ext4 bs=1M count=64
  mkfs.ext4 -L bootfs bootfs.ext4
  sudo mount -o loop bootfs.ext4 /mnt/bootfs
  sudo cp /srv/tftp/uImage /srv/tftp/stm32mp157d-andy.dtb /mnt/bootfs/
  sync && sudo umount /mnt/bootfs
```
# program to sdcard
sdcard.tsv
```sh
#Opt    Id      Name    Type    Device  Offset  Binary
-       0x01    fsbl1-boot      Binary  none    0x0     serialboot-me.stm32
-       0x03    ssbl-boot       Binary  none    0x0     u-boot-st.stm32
P       0x04    fsbl1   Binary  mmc0    0x00004400      tf-a-me.stm32
P       0x05    fsbl2   Binary  mmc0    0x00044400      tf-a-me.stm32
P       0x06    ssbl    Binary  mmc0    0x00084400      u-boot-st.stm32
P       0x21    boot    System  mmc0    0x00284400      bootfs/bootfs.ext4
PE      0x22    rootfs  FileSystem      mmc0    0x04284400      none
#P      0x22    rootfs  FileSystem      mmc0    0x04284400      atk-image-qt5.12.9-rootfs.ext4
```
# boot
```sh
STM32MP> mmc part
STM32MP> ext4ls mmc 0:4
<DIR>       4096 .
<DIR>       4096 ..
<DIR>      16384 lost+found
         7332408 uImage
           63833 stm32mp157d-andy.dtb
STM32MP> print bootargs
bootargs=console=ttySTM0,115200 root=/dev/nfs nfsroot=192.168.1.43:/srv/nfs/rootfs ip=dhcp
STM32MP> setenv bootcmd 'ext4load mmc 0:4 c2000000 uImage;ext4load mmc 0:4 c4000000 stm32mp157d-andy.dtb;bootm c2000000 - c4000000'
STM32MP> saveenv
STM32MP> boot
```
