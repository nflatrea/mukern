/* arch/x86_64/trap/idt.h - Interrupt Descriptor Table + trap frame (M1b). */
#pragma once
#include <stdint.h>

/* 64-bit IDT gate descriptor (16 bytes). */
struct idt_entry {
    uint16_t off_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t off_mid;
    uint32_t off_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Register snapshot built by the ISR stubs (isr.S). Field order must match
 * the push sequence in isr_common exactly. */
struct trapframe {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vec, err;                  /* pushed by stub                  */
    uint64_t rip, cs, rflags, rsp, ss;  /* pushed by CPU on interrupt      */
} __attribute__((packed));

#define IDT_GATE_INT64  0x8E            /* present, DPL0, 64-bit interrupt */
#define KERNEL_CS       0x08

void idt_set_gate(int n, uint64_t handler, uint16_t sel, uint8_t flags);
void idt_init(void);
