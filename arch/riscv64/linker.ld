/* arch/riscv64/linker.ld - OpenSBI jumps to the payload at 0x80200000. */
ENTRY(_start)

SECTIONS
{
    . = 0x80200000;

    .text   : { KEEP(*(.text.boot)) *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    .bss    : { *(.bss*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.note*) }
}
