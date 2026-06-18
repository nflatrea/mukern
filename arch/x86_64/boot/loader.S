/* arch/x86_64/boot/loader.S
 *
 * muKern x86_64 long-mode bring-up (M1a).
 *
 * Entered from header.S in 32-bit protected mode. Steps:
 *   1. Build identity-mapped page tables for the first 1 GiB (2 MiB pages).
 *   2. Load CR3, enable PAE, set EFER.LME, enable paging  -> long mode.
 *   3. Load a flat 64-bit GDT and far-jump into 64-bit code.
 *   4. Reload segments, set up the stack, and call kmain().
 */

/* ---- Page tables (one each: PML4, PDPT, PD), 4 KiB aligned ---- */
.section .bss
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd:   .skip 4096

/* ---- Flat 64-bit GDT: null, kernel code, kernel data ---- */
.section .rodata
.align 16
gdt64:
    .quad 0                                              /* 0x00: null   */
gdt64_code = . - gdt64
    .quad (1<<43)|(1<<44)|(1<<47)|(1<<53)                /* 0x08: code   */
gdt64_data = . - gdt64
    .quad (1<<41)|(1<<44)|(1<<47)                        /* 0x10: data   */
gdt64_pointer:
    .word . - gdt64 - 1
    .quad gdt64

/* ============================ 32-bit code ============================ */
.section .text
.code32
.global boot_long_mode
.type boot_long_mode, @function
boot_long_mode:
    /* pml4[0] = pdpt | (present | writable) */
    movl $pdpt, %eax
    orl  $0x3, %eax
    movl %eax, pml4

    /* pdpt[0] = pd | (present | writable) */
    movl $pd, %eax
    orl  $0x3, %eax
    movl %eax, pdpt

    /* Fill the page directory with 512 * 2 MiB identity-mapped pages. */
    xorl %ecx, %ecx
.fill_pd:
    movl $0x200000, %eax             /* 2 MiB                            */
    mull %ecx                        /* edx:eax = 2 MiB * ecx (<1 GiB)   */
    orl  $0x83, %eax                 /* present | writable | page-size   */
    movl %eax, pd(, %ecx, 8)         /* low dword of entry ecx           */
    movl $0,   pd + 4(, %ecx, 8)     /* high dword                       */
    incl %ecx
    cmpl $512, %ecx
    jne  .fill_pd

    /* CR3 = PML4 */
    movl $pml4, %eax
    movl %eax, %cr3

    /* CR4.PAE = 1 */
    movl %cr4, %eax
    orl  $(1<<5), %eax
    movl %eax, %cr4

    /* EFER.LME = 1  (MSR 0xC0000080, bit 8) */
    movl $0xC0000080, %ecx
    rdmsr
    orl  $(1<<8), %eax
    wrmsr

    /* CR0.PG = 1  -> paging on, now in long (compatibility) mode */
    movl %cr0, %eax
    orl  $(1<<31), %eax
    movl %eax, %cr0

    /* Load 64-bit GDT and far-jump into the 64-bit code segment. */
    lgdt gdt64_pointer
    ljmp $gdt64_code, $long_mode_start
.size boot_long_mode, . - boot_long_mode

/* ============================ 64-bit code ============================ */
.code64
.global long_mode_start
.type long_mode_start, @function
long_mode_start:
    movw $gdt64_data, %ax            /* reload all data segments         */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    leaq boot_stack_top(%rip), %rsp  /* 64-bit stack                     */
    xorq %rbp, %rbp                  /* terminate stack-frame chain      */

    call kmain                       /* enter C; should not return       */
.hang64:
    cli
    hlt
    jmp .hang64
.size long_mode_start, . - long_mode_start
