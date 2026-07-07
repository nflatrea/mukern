/* arch/x86_64/trap/gdt.h - GDT + TSS setup for user mode (M3c). */
#pragma once
#include <stdint.h>

/* Segment selectors in the runtime GDT. */
#define SEL_KCODE 0x08
#define SEL_KDATA 0x10
#define SEL_UCODE 0x18          /* | RPL 3 -> 0x1B */
#define SEL_UDATA 0x20          /* | RPL 3 -> 0x23 */
#define SEL_TSS   0x28

void gdt_init(void);
void arch_set_kernel_stack(uint64_t rsp0);   /* sets TSS.rsp0 */
