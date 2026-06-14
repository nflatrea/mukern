/*
arch/x86_64/boot.S

Multiboot1 entry, protected to long mode.
QEMU's `-kernel` loader recognises the Multiboot1 header and enters at
_start in 32-bit protected mode, paging off. We identity-map the first
1 GiB with 2 MiB pages, switch to long mode, and call kmain().
*/

.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    0x0
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

.section .bss, "aw", @nobits
.balign 4096
pml4: .skip 4096
pdpt: .skip 4096
pd:   .skip 4096
.balign 16
stack_bottom:
.skip 0x4000           /* 16 KiB */
stack_top:

.section .rodata
.balign 8
gdt64:
    .quad 0                                         /* null */
    .set gdt64_code, . - gdt64
    .quad (1<<43)|(1<<44)|(1<<47)|(1<<53)           /* code: X, S, P, L */
    .set gdt64_data, . - gdt64
    .quad (1<<44)|(1<<47)|(1<<41)                   /* data: S, P, W     */
gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global _start
_start:
    mov $stack_top, %esp

    /* PML4[0] -> PDPT */
    mov $pdpt, %eax
    or  $0x3, %eax            /* present | writable */
    mov %eax, pml4

    /* PDPT[0] -> PD */
    mov $pd, %eax
    or  $0x3, %eax
    mov %eax, pdpt

    /* PD: 512 * 2 MiB identity pages (covers low 1 GiB) */
    mov $pd, %edi
    xor %ecx, %ecx
    mov $0x83, %ebx          /* present | writable | page-size(2MiB), addr 0 */
1:
    mov %ebx, (%edi)
    movl $0, 4(%edi)         /* high dword: addresses < 4 GiB */
    add $0x200000, %ebx
    add $8, %edi
    inc %ecx
    cmp $512, %ecx
    jne 1b

    /* load CR3 */
    mov $pml4, %eax
    mov %eax, %cr3

    /* CR4.PAE */
    mov %cr4, %eax
    or  $(1<<5), %eax
    mov %eax, %cr4

    /* EFER.LME */
    mov $0xC0000080, %ecx
    rdmsr
    or  $(1<<8), %eax
    wrmsr

    /* CR0.PG (paging on; PE already set by multiboot) */
    mov %cr0, %eax
    or  $(1<<31), %eax
    mov %eax, %cr0

    lgdt gdt64_pointer
    ljmp $gdt64_code, $long_mode

.code64
long_mode:
    mov $gdt64_data, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    mov $stack_top, %rsp
    call kmain
.hang:
    cli
    hlt
    jmp .hang
