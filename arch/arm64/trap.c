/* arch/arm64/trap.c - VBAR_EL1 setup + fatal-trap dump. */
#include <stdint.h>
#include "arch.h"
#include "klib.h"

struct aregs {
    uint64_t x[31];    /* x0..x30 at offsets 0..240 */
    uint64_t elr;      /* 248 */
    uint64_t spsr;     /* 256 */
    uint64_t esr;      /* 264 */
    uint64_t far;      /* 272 */
};

static const char *ec_name(unsigned ec)
{
    switch (ec) {
    case 0x15: return "SVC (AArch64)";
    case 0x3C: return "BRK (breakpoint)";
    case 0x24: return "data abort (lower EL)";
    case 0x25: return "data abort (same EL)";
    case 0x20: return "instruction abort (lower EL)";
    case 0x21: return "instruction abort (same EL)";
    case 0x00: return "unknown";
    default:   return "exception";
    }
}

void arch_traps_init(void)
{
    extern char vectors[];
    __asm__ volatile("msr vbar_el1, %0; isb" : : "r"(vectors));
}

void arm_trap(struct aregs *r)
{
    unsigned ec = (unsigned)((r->esr >> 26) & 0x3F);
    kprintf("\n*** AArch64 exception ***\n");
    kprintf("ESR=%016lx  EC=0x%02x (%s)\n", r->esr, ec, ec_name(ec));
    kprintf("ELR=%016lx  FAR=%016lx  SPSR=%016lx\n", r->elr, r->far, r->spsr);
    kprintf("x0=%016lx x1=%016lx x2=%016lx x3=%016lx\n",
            r->x[0], r->x[1], r->x[2], r->x[3]);
    kprintf("x29(fp)=%016lx x30(lr)=%016lx\n", r->x[29], r->x[30]);
    panic("unhandled AArch64 exception (EC=0x%02x)", ec);
}

void arch_trigger_fault(void)
{
    __asm__ volatile("brk #0");   /* synchronous breakpoint exception */
}
