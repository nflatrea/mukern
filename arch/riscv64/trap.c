/* arch/riscv64/trap.c - stvec setup + fatal-trap dump. */

#include <stdint.h>
#include "arch.h"
#include "klib.h"

struct rregs {
    uint64_t x[32];    /* x0..x31 at offsets 0..248 (x0 unused) */
    uint64_t sepc;     /* 256 */
    uint64_t scause;   /* 264 */
    uint64_t stval;    /* 272 */
};

static const char *exc_name(unsigned long code)
{
    switch (code) {
    case 0:  return "instruction address misaligned";
    case 1:  return "instruction access fault";
    case 2:  return "illegal instruction";
    case 3:  return "breakpoint";
    case 5:  return "load access fault";
    case 7:  return "store access fault";
    case 8:  return "ecall from U-mode";
    case 9:  return "ecall from S-mode";
    case 12: return "instruction page fault";
    case 13: return "load page fault";
    case 15: return "store page fault";
    default: return "exception";
    }
}

void arch_traps_init(void)
{
    extern char trap_vector[];
    /* direct mode: low two bits of stvec are 0 */
    __asm__ volatile("csrw stvec, %0" : : "r"(trap_vector));
}

void riscv_trap(struct rregs *r)
{
    int is_irq = (int)(r->scause >> 63);
    unsigned long code = r->scause & 0x7FFFFFFFFFFFFFFFUL;

    kprintf("\n*** RISC-V trap ***\n");
    kprintf("scause=%016lx  sepc=%016lx  stval=%016lx\n",
            r->scause, r->sepc, r->stval);
    if (is_irq)
        kprintf("interrupt, code=%lu\n", code);
    else
        kprintf("exception: %s (code=%lu)\n", exc_name(code), code);
    kprintf("ra=%016lx sp=%016lx a0=%016lx a1=%016lx\n",
            r->x[1], r->x[2], r->x[10], r->x[11]);

    panic("unhandled RISC-V %s (code=%lu)",
          is_irq ? "interrupt" : "exception", code);
}

void arch_trigger_fault(void)
{
    __asm__ volatile("unimp");   /* illegal instruction */
}
