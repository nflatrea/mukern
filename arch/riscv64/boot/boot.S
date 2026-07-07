/* arch/riscv64/boot/boot.S - S-mode entry. OpenSBI (M-mode firmware) loads the
 * kernel at 0x80200000 and jumps here with a0=hartid, a1=devicetree.
 */
.section .text.boot
.global _start
_start:
    la      sp, _stack_top          /* boot/idle stack */
    /* zero .bss */
    la      t0, _bss_start
    la      t1, _bss_end
1:  bgeu    t0, t1, 2f
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       1b
2:  call    kmain
3:  wfi
    j       3b
