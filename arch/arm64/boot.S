/*
arch/arm64/boot.S

AArch64 entry for QEMU `-M virt`.
QEMU loads the ELF and enters at _start (EL1 by default). We park
secondary CPUs, set a stack, and call kmain(). The PL011 UART works
at any EL, so no further setup is needed for the PoC.
*/

.section .text.boot
.global _start
_start:
    mrs   x1, mpidr_el1          /* park anything but CPU0 */
    and   x1, x1, #0xff
    cbnz  x1, park

    ldr   x0, =stack_top
    mov   sp, x0
    bl    kmain

park:
1:  wfe
    b     1b

.section .bss
.balign 16
stack_bottom:
.skip 0x4000                     /* 16 KiB */
stack_top:
