/* arch/arm64/linker.ld - QEMU virt loads kernels at 0x40080000. */
ENTRY(_start)

SECTIONS
{
    . = 0x40080000;

    .text   : { KEEP(*(.text.boot)) *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    .bss    : { *(.bss*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.note*) }
}
