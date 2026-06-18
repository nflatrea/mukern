/* arch/x86_64/trap/idt.c - build and load the IDT (M1b). */
#include "arch/x86_64/trap/idt.h"

/* Stub entry points, generated in isr.S, one per vector 0..47. */
extern uint64_t isr_stub_table[];
#define NUM_STUBS 48

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

void idt_set_gate(int n, uint64_t handler, uint16_t sel, uint8_t flags)
{
    idt[n].off_low   = (uint16_t)(handler & 0xFFFF);
    idt[n].selector  = sel;
    idt[n].ist       = 0;
    idt[n].type_attr = flags;
    idt[n].off_mid   = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[n].off_high  = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[n].zero      = 0;
}

void idt_init(void)
{
    for (int i = 0; i < NUM_STUBS; i++)
        idt_set_gate(i, isr_stub_table[i], KERNEL_CS, IDT_GATE_INT64);

    idtp.limit = (uint16_t)(sizeof(idt) - 1);
    idtp.base  = (uint64_t)&idt;
    __asm__ volatile ("lidt %0" :: "m"(idtp));
}
