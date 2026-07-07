/* arch/x86_64/mm/memmap.c - discover usable RAM from the Multiboot1 map.
 *
 * header.S stashed the Multiboot info pointer in `mb_info`. With our header
 * flags the bootloader provides a memory map; we walk it and report the
 * available (type 1) regions to the generic frame allocator.
 */
#include "vm.h"

/* Set by arch/x86_64/boot/header.S (physical address, identity-mapped). */
extern volatile uint32_t mb_info;

struct mb_info_hdr {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

struct mb_mmap_entry {
    uint32_t size;          /* size of this entry, excluding this field */
    uint64_t base;
    uint64_t len;
    uint32_t type;          /* 1 == available RAM */
} __attribute__((packed));

#define MB_FLAG_MMAP   (1u << 6)
#define MB_FLAG_MEM    (1u << 0)

int arch_mem_detect(struct mem_region *regions, int max, phys_addr_t *ram_top)
{
    struct mb_info_hdr *mbi = (struct mb_info_hdr *)(uintptr_t)mb_info;
    int n = 0;
    phys_addr_t top = 0;

    if (mbi && (mbi->flags & MB_FLAG_MMAP) && mbi->mmap_length) {
        uintptr_t p   = (uintptr_t)mbi->mmap_addr;
        uintptr_t end = p + mbi->mmap_length;
        while (p < end && n < max) {
            struct mb_mmap_entry *e = (struct mb_mmap_entry *)p;
            if (e->type == 1 && e->len) {
                regions[n].base = e->base;
                regions[n].len  = e->len;
                if (e->base + e->len > top)
                    top = e->base + e->len;
                n++;
            }
            p += e->size + 4;               /* size excludes its own field */
        }
    } else if (mbi && (mbi->flags & MB_FLAG_MEM) && max > 0) {
        /* Fallback: a single region above 1 MiB from mem_upper (KiB). */
        regions[0].base = 0x100000;
        regions[0].len  = (uint64_t)mbi->mem_upper * 1024;
        top = regions[0].base + regions[0].len;
        n = 1;
    }

    if (ram_top)
        *ram_top = top;
    return n;
}
