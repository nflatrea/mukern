/* arch/x86_64/mm/paging.c - x86_64 4-level paging (M2b).
 *
 * Phase 1 brought the MMU up with a 2 MiB identity map (loader.S). This adds
 * 4 KiB-granular page-table management on the *active* address space: walk
 * PML4 -> PDPT -> PD -> PT, allocating intermediate tables from the PMM.
 * Low physical RAM is identity-mapped, so a physical frame address doubles
 * as a pointer to that frame.
 */
#include "vm.h"

#define PTE_PRESENT (1ull << 0)
#define PTE_WRITE   (1ull << 1)
#define PTE_USER    (1ull << 2)
#define PTE_PS      (1ull << 7)            /* large page (no child table) */
#define ADDR_MASK   0x000FFFFFFFFFF000ull

static inline uint64_t *phys_ptr(phys_addr_t p) { return (uint64_t *)(uintptr_t)p; }

static inline phys_addr_t read_cr3(void)
{
    uint64_t v;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(v));
    return v & ADDR_MASK;
}

void arch_tlb_flush(virt_addr_t va)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(va) : "memory");
}

phys_addr_t arch_fault_addr(void)
{
    uint64_t v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

static uint64_t flags_to_pte(uint64_t f)
{
    uint64_t p = PTE_PRESENT;
    if (f & VM_WRITE) p |= PTE_WRITE;
    if (f & VM_USER)  p |= PTE_USER;
    return p;
}

/* Return the child table for table[idx], creating it if requested. */
static uint64_t *next_table(uint64_t *table, int idx, int create)
{
    if (!(table[idx] & PTE_PRESENT)) {
        if (!create)
            return 0;
        phys_addr_t f = pmm_alloc_frame();
        if (!f)
            return 0;
        uint64_t *nt = phys_ptr(f);
        for (int i = 0; i < 512; i++)
            nt[i] = 0;
        table[idx] = f | PTE_PRESENT | PTE_WRITE | PTE_USER;
        return nt;
    }
    if (table[idx] & PTE_PS)
        return 0;                          /* large page: refuse to split */
    return phys_ptr(table[idx] & ADDR_MASK);
}

int vmm_map(virt_addr_t va, phys_addr_t pa, uint64_t flags)
{
    uint64_t *pml4 = phys_ptr(read_cr3());
    int i4 = (va >> 39) & 0x1FF;
    int i3 = (va >> 30) & 0x1FF;
    int i2 = (va >> 21) & 0x1FF;
    int i1 = (va >> 12) & 0x1FF;

    uint64_t *pdpt = next_table(pml4, i4, 1); if (!pdpt) return -1;
    uint64_t *pd   = next_table(pdpt, i3, 1); if (!pd)   return -1;
    uint64_t *pt   = next_table(pd,   i2, 1); if (!pt)   return -1;

    pt[i1] = (pa & ADDR_MASK) | flags_to_pte(flags);
    arch_tlb_flush(va);
    return 0;
}

void vmm_unmap(virt_addr_t va)
{
    uint64_t *pml4 = phys_ptr(read_cr3());
    int i4 = (va >> 39) & 0x1FF;
    int i3 = (va >> 30) & 0x1FF;
    int i2 = (va >> 21) & 0x1FF;
    int i1 = (va >> 12) & 0x1FF;

    uint64_t *pdpt = next_table(pml4, i4, 0); if (!pdpt) return;
    uint64_t *pd   = next_table(pdpt, i3, 0); if (!pd)   return;
    uint64_t *pt   = next_table(pd,   i2, 0); if (!pt)   return;

    pt[i1] = 0;
    arch_tlb_flush(va);
}
