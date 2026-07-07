/* include/vm.h - physical frame allocator + virtual memory (paging) API. */
#pragma once
#include "types.h"

/* ---- Memory map (filled by the arch from firmware/bootloader data) ---- */
struct mem_region {
    phys_addr_t base;
    uint64_t    len;
};

/* Architecture hook: discover usable RAM. Returns the number of regions
 * written (<= max). *ram_top receives the highest usable physical address. */
int arch_mem_detect(struct mem_region *regions, int max, phys_addr_t *ram_top);

/* ---- M2a: physical frame allocator (bitmap, in kernel/vm.c) ---- */
void        pmm_init(void);
phys_addr_t pmm_alloc_frame(void);          /* returns 0 on out-of-memory */
void        pmm_free_frame(phys_addr_t frame);
uint64_t    pmm_free_count(void);           /* free frames remaining      */
uint64_t    pmm_total_count(void);          /* total managed frames       */

/* ---- M2b: virtual memory / 4-level paging (arch/x86_64/mm/paging.c) ---- */
#define VM_PRESENT  (1u << 0)
#define VM_WRITE    (1u << 1)
#define VM_USER     (1u << 2)

/* Map/unmap a single 4 KiB page in the currently active address space.
 * vmm_map allocates intermediate page tables from the PMM as needed. */
int  vmm_map(virt_addr_t va, phys_addr_t pa, uint64_t flags);
void vmm_unmap(virt_addr_t va);

/* Read the faulting address register (CR2 on x86_64). */
phys_addr_t arch_fault_addr(void);

/* Flush a single TLB entry. */
void arch_tlb_flush(virt_addr_t va);

/* ---- M2b demo: a demand-paged virtual region. Touching an address here
 * faults; the page-fault handler maps a fresh frame on demand (a controlled
 * fault), foreshadowing the external pager of Phase 4. ---- */
#define VM_DEMAND_BASE 0xFFFFFFFF40000000ull
#define VM_DEMAND_SIZE 0x0000000000400000ull   /* 4 MiB */
