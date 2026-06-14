/*
arch/riscv64/boot.S

RV64 entry. OpenSBI (-bios default) hands off in
S-mode at 0x80200000 with a0=hartid, a1=dtb. We park non-zero harts,
set a stack, and call kmain().
*/

.section .text.boot
.global _start
_start:
    bnez  a0, park           /* only hart 0 proceeds */
    la    sp, stack_top
    call  kmain

park:
1:  wfi
    j     1b

.section .bss
.balign 16
stack_bottom:
.skip 0x4000                 /* 16 KiB */
stack_top:
