/* arch/x86_64/linker.ld - load at 1 MiB; multiboot header first. */
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot : { KEEP(*(.multiboot)) }
    .text      : { *(.text*) }
    .rodata    : { *(.rodata*) }
    .data      : { *(.data*) }
    .bss       : { *(.bss*) *(COMMON) }

    /DISCARD/  : { *(.comment) *(.note*) *(.eh_frame*) }
}
