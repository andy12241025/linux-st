# Practice atop st linux v5.4.31

`build.sh` drives out-of-tree builds in `build/` using `stm32mp1_andy_defconfig`.

```sh
./build.sh          # cleanup + configure + build
./build.sh -r       # cleanup only
./build.sh -c       # configure only
./build.sh -m       # menuconfig (requires existing .config)
./build.sh -b       # build only (requires existing .config)
./build.sh -c -m    # configure, then menuconfig
```

Regenerate `stm32mp1_andy_defconfig` after changing fragments:

```sh
./gen_defconfig.sh
```


