# kernel module SegV debugging
For error output:
```sh
# modprobe andyled
[   26.796530] andyled: module verification failed: signature and/or required key missing - tainting kernel
[   26.805218] andy_led node found!
[   26.807924] compatible = andy-leds
[   26.811337] status = okay
[   26.814026] reg property read failed!
[   26.817865] 8<--- cut here ---
[   26.820808] Unable to handle kernel NULL pointer dereference at virtual address 00000000
[   26.828938] pgd = f423a498
[   26.831582] [00000000] *pgd=00000000
[   26.835150] Internal error: Oops: 5 [#1] PREEMPT SMP ARM
[   26.840447] Modules linked in: andyled(E+)
[   26.844539] CPU: 1 PID: 178 Comm: modprobe Tainted: G            E     5.4.31 #5
[   26.851918] Hardware name: STM32 (Device Tree Support)
[   26.857067] PC is at led_init+0x1c4/0x1000 [andyled]
[   26.862011] LR is at _raw_spin_unlock_irqrestore+0x28/0x50
[   26.867478] pc : [<bf0051c4>]    lr : [<c0b58194>]    psr: 600f0013
[   26.873735] sp : ed0abe48  ip : 00000000  fp : 004895fc
[   26.878950] r10: 0000017b  r9 : ed0aa000  r8 : c0101204
[   26.884166] r7 : 00000000  r6 : ffffe000  r5 : bf005000  r4 : bf0022c0
[   26.890685] r3 : 00000000  r2 : 00000000  r1 : 800f0013  r0 : 00000000
[   26.897205] Flags: nZCv  IRQs on  FIQs on  Mode SVC_32  ISA ARM  Segment none
[   26.904332] Control: 10c5387d  Table: edfe406a  DAC: 00000051
[   26.910068] Process modprobe (pid: 178, stack limit = 0xff3c528a)
[   26.916151] Stack: (0xed0abe48 to 0xed0ac000)
[   26.920502] be40:                   00000000 38e38e39 cfffc23c c11accb4 efff1470 efeb8fc0
[   26.928673] be60: 8040003f 676097dd 00000000 efd640e4 edab1940 00000001 c11050c8 c02b4480
[   26.936842] be80: efd640e4 676097dd c01c2090 c11b0ba0 bf005000 ffffe000 00000000 c0103098
[   26.945011] bea0: 00000001 c01c2090 00000000 bf002080 ed0abf40 c1104cac c0c043b4 00000000
[   26.953181] bec0: c1104dc0 c01c2090 ffff8000 00007fff bf002080 c01bd170 c01c0bb4 00000008
[   26.961350] bee0: c0dedfe4 bf002080 004860b4 676097dd 0000017b bf002080 edac4e40 00000003
[   26.969519] bf00: 0000017b c01c0bec 0000017b c0101204 00000000 004860b4 00000003 c01c2514
[   26.977690] bf20: 7fffffff 00000000 00000003 fffff000 00000fff f0c21000 00027f24 00000000
[   26.985859] bf40: f0c2187b f0c21f04 f0c21000 00027f24 f0c487a4 f0c485c4 f0c3fb94 00003000
[   26.994028] bf60: 000030d0 00001b30 00003132 00000000 00000000 00000000 00001b20 0000002d
[   27.002199] bf80: 0000002e 00000018 00000015 00000014 00000000 676097dd 00000000 00000001
[   27.010369] bfa0: 0048c158 c0101000 00000000 00000001 00000003 004860b4 00000000 00000000
[   27.018537] bfc0: 00000000 00000001 0048c158 0000017b 0048c1e8 00000001 00000000 004895fc
[   27.026707] bfe0: bec9d998 bec9d988 0047caa8 b6f08a72 600f0030 00000003 00000000 00000000
[   27.034895] [<bf0051c4>] (led_init [andyled]) from [<c0103098>] (do_one_initcall+0x54/0x2a4)
[   27.043313] [<c0103098>] (do_one_initcall) from [<c01c0bec>] (do_init_module+0x5c/0x288)
[   27.051389] [<c01c0bec>] (do_init_module) from [<c01c2514>] (sys_finit_module+0xe4/0x120)
[   27.059555] [<c01c2514>] (sys_finit_module) from [<c0101000>] (ret_fast_syscall+0x0/0x54)
[   27.067717] Exception stack(0xed0abfa8 to 0xed0abff0)
[   27.072762] bfa0:                   00000000 00000001 00000003 004860b4 00000000 00000000
[   27.080933] bfc0: 00000000 00000001 0048c158 0000017b 0048c1e8 00000001 00000000 004895fc
[   27.089096] bfe0: bec9d998 bec9d988 0047caa8 b6f08a72
[   27.094141] Code: e5843068 eb64f44d e5943058 e5840054 (e5933000)
[   27.100302] ---[ end trace 60a28f461a4108ec ]---
Segmentation fault
#
```

based on:
```sh
[   26.857067] PC is at led_init+0x1c4/0x1000 [andyled]
```

search objdump results for led_init+0x1c4 (could be verified by code e5933000):
```sh
❯ arm-none-linux-gnueabihf-objdump -dS andyled.ko | less
 1c0:   e5840054        str     r0, [r4, #84]   @ 0x54
        asm volatile("ldr %0, %1"
 1c4:   e5933000        ldr     r3, [r3]
        val = readl(MPU_AHB4_PERIPH_RCC_PI);
 1c8:   f57ff04f        dsb     sy
```
