/* arch/x86_64/boot/header.S
 *
 * muKern x86_64 boot entry (M1a).
 *
 * A Multiboot1-compliant bootloader (GRUB, or QEMU's built-in `-kernel`
 * loader) hands us control in 32-bit protected mode with paging disabled.
 * This file provides the Multiboot header so we are recognised, sets up a
 * temporary boot stack, stashes the Multiboot info pointer, and hands off to
 * the long-mode bring-up code in loader.S.
 */

/* ---- Multiboot1 header (must live in the first 8 KiB of the image) ---- */
.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    (1 << 0) | (1 << 1)        /* align modules | provide mem map */
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot_header, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

/* ---- Temporary boot stack (16 KiB) ---- */
.section .bss
.align 16
.global boot_stack_bottom
boot_stack_bottom:
.skip 16384
.global boot_stack_top
boot_stack_top:

/* ---- Saved Multiboot handoff values ---- */
.section .data
.global mb_magic
.global mb_info
mb_magic: .long 0
mb_info:  .long 0

/* ---- 32-bit entry point ---- */
.section .text
.code32
.global _start
.type _start, @function
_start:
    movl $boot_stack_top, %esp       /* set up a usable stack            */
    movl %eax, mb_magic              /* preserve Multiboot magic         */
    movl %ebx, mb_info               /* preserve Multiboot info pointer  */
    jmp  boot_long_mode              /* bring up long mode (loader.S)     */
.hang32:
    cli
    hlt
    jmp .hang32
.size _start, . - _start
