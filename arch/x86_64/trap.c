/* arch/x86_64/trap.c - IDT setup + fatal-trap dump. */
#include <stdint.h>
#include "arch.h"
#include "klib.h"

struct trapframe {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, errcode;
    uint64_t rip, cs, rflags, rsp, ss;
};

struct idt_entry {
    uint16_t off_lo;
    uint16_t sel;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t off_mid;
    uint32_t off_hi;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
extern void *isr_table[];   /* from trap.S */

static const char *exname(uint64_t v)
{
    switch (v) {
    case 0:  return "#DE divide error";
    case 3:  return "#BP breakpoint";
    case 6:  return "#UD invalid opcode";
    case 8:  return "#DF double fault";
    case 13: return "#GP general protection";
    case 14: return "#PF page fault";
    default: return "exception";
    }
}

static void set_gate(int n, void *handler)
{
    uint64_t a = (uint64_t)handler;
    idt[n].off_lo  = a & 0xFFFF;
    idt[n].sel     = 0x08;       /* kernel code selector from boot GDT */
    idt[n].ist     = 0;
    idt[n].flags   = 0x8E;       /* present, DPL0, 64-bit interrupt gate */
    idt[n].off_mid = (a >> 16) & 0xFFFF;
    idt[n].off_hi  = (a >> 32);
    idt[n].zero    = 0;
}

void arch_traps_init(void)
{
    for (int i = 0; i < 32; i++)
        set_gate(i, isr_table[i]);

    struct idt_ptr p = { sizeof(idt) - 1, (uint64_t)idt };
    __asm__ volatile("lidt %0" : : "m"(p));
}

void x86_trap(struct trapframe *tf)
{
    kprintf("\n*** x86_64 exception ***\n");
    kprintf("vec=%lu (%s)  err=0x%lx  rip=%016lx\n",
            tf->vector, exname(tf->vector), tf->errcode, tf->rip);
    kprintf("cs=%lx rflags=%016lx rsp=%016lx\n", tf->cs, tf->rflags, tf->rsp);
    kprintf("rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
            tf->rax, tf->rbx, tf->rcx, tf->rdx);
    kprintf("rsi=%016lx rdi=%016lx rbp=%016lx\n", tf->rsi, tf->rdi, tf->rbp);
    panic("unhandled x86_64 exception (vec=%lu)", tf->vector);
}

void arch_trigger_fault(void)
{
    __asm__ volatile("ud2");   /* #UD, vector 6 */
}
