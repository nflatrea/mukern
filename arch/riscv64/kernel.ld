OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
    . = 0x80200000;
    .text   : { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data   : { *(.data .data.*) *(.sdata .sdata.*) }
    . = ALIGN(16);
    _bss_start = .;
    .bss    : { *(.sbss .sbss.*) *(.bss .bss.*) *(COMMON) }
    . = ALIGN(16);
    _bss_end = .;
    . += 0x4000;                    /* 16 KiB boot/idle stack */
    . = ALIGN(16);
    _stack_top = .;
}
