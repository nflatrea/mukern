/* user/user.ld - flat user-program layout. The base VA is supplied with
 * --defsym UBASE=0x...; crt0's _ustart (in .text.start) is forced first so the
 * program's entry point equals its load base, which is all the kernel loader
 * needs to know. */
ENTRY(_ustart)
SECTIONS {
    . = UBASE;
    .text : {
        *(.text.start)
        *(.text .text.* .text.startup .text.startup.*)
    }
    .rodata : { *(.rodata .rodata.*) }
    .data   : { *(.data .data.*) }
    .bss    : { *(.bss .bss.*) *(COMMON) }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.eh_frame_hdr) }
}
