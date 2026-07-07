/* arch/x86_64/trap/gdt.c - runtime GDT with user segments + TSS (M3c).
 *
 * Phase 1's GDT (in loader.S) had only kernel code/data. To enter Ring 3 and
 * to take traps from Ring 3 we need user code/data descriptors and a TSS
 * whose RSP0 names the kernel stack to switch to on a privilege change.
 * Kernel selectors keep their Phase 1 values, so CS/DS/SS stay valid across
 * the lgdt without reloading.
 */
#include "arch/x86_64/trap/gdt.h"

struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

static uint64_t gdt[7];          /* null,kcode,kdata,ucode,udata,TSS(2 slots) */
static struct tss tss;
static struct { uint16_t limit; uint64_t base; } __attribute__((packed)) gdtr;

static void set_tss_desc(int idx, uint64_t base, uint32_t limit)
{
    uint64_t lo = 0;
    lo |= (uint64_t)(limit & 0xFFFF);
    lo |= (base & 0xFFFFFFull) << 16;
    lo |= (uint64_t)0x89 << 40;             /* present | type=9 (avail TSS) */
    lo |= (uint64_t)((limit >> 16) & 0xF) << 48;
    lo |= ((base >> 24) & 0xFFull) << 56;
    gdt[idx]     = lo;
    gdt[idx + 1] = (base >> 32) & 0xFFFFFFFFull;
}

void gdt_init(void)
{
    gdt[0] = 0;
    gdt[1] = (1ull<<43)|(1ull<<44)|(1ull<<47)|(1ull<<53);              /* kcode */
    gdt[2] = (1ull<<41)|(1ull<<44)|(1ull<<47);                         /* kdata */
    gdt[3] = (1ull<<43)|(1ull<<44)|(1ull<<47)|(1ull<<53)|(3ull<<45);   /* ucode */
    gdt[4] = (1ull<<41)|(1ull<<44)|(1ull<<47)|(3ull<<45);              /* udata */

    for (unsigned i = 0; i < sizeof(tss) / 8; i++)
        ((uint64_t *)&tss)[i] = 0;
    tss.rsp0       = 0;                      /* set per-thread before user runs */
    tss.iomap_base = sizeof(tss);
    set_tss_desc(5, (uint64_t)&tss, sizeof(tss) - 1);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)gdt;
    __asm__ volatile ("lgdt %0" :: "m"(gdtr));
    __asm__ volatile ("ltr %%ax" :: "a"((uint16_t)SEL_TSS));
}

void arch_set_kernel_stack(uint64_t rsp0)
{
    tss.rsp0 = rsp0;
}
