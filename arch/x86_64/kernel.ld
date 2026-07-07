/* arch/x86_64/kernel.ld - link map for the muKern x86_64 kernel.
 *
 * Loaded by a Multiboot bootloader at the conventional 1 MiB mark. The
 * Multiboot header is placed first so it falls within the first 8 KiB of
 * the image. Notes/unwind tables are discarded to keep the loader's image
 * probing unambiguous (so QEMU treats us as Multiboot, not PVH).
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .boot ALIGN(8) :
    {
        KEEP(*(.multiboot_header))
    }

    .text ALIGN(4K) :
    {
        *(.text .text.*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) :
    {
        *(.data .data.*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    /* End of the kernel image; the physical frame allocator places its
     * bitmap here and treats everything below as reserved. */
    . = ALIGN(4K);
    kernel_end = .;

    /DISCARD/ :
    {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
    }
}
