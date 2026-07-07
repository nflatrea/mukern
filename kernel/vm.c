/* kernel/vm.c - physical frame allocator (M2a).
 *
 * A bitmap of all physical frames: bit set == in use. Mechanism only; no
 * paging policy lives here (that belongs in user space from Phase 4). The
 * bitmap is placed immediately after the kernel image (linker `kernel_end`)
 * and everything below it is treated as reserved (low memory + kernel).
 */
#include "vm.h"

#define MAX_REGIONS 32

extern char kernel_end[];          /* linker symbol: end of kernel image */

static uint8_t  *bitmap;           /* 1 bit per frame                    */
static uint64_t  total_frames;
static uint64_t  free_frames;
static uint64_t  alloc_hint;       /* search cursor for the next alloc   */

static inline void bm_set(uint64_t f)   { bitmap[f >> 3] |=  (uint8_t)(1u << (f & 7)); }
static inline void bm_clear(uint64_t f) { bitmap[f >> 3] &= (uint8_t)~(1u << (f & 7)); }
static inline int  bm_test(uint64_t f)  { return bitmap[f >> 3] &   (1u << (f & 7)); }

static void mark_used(uint64_t f)
{
    if (f < total_frames && !bm_test(f)) {
        bm_set(f);
        free_frames--;
    }
}

static void mark_free(uint64_t f)
{
    if (f < total_frames && bm_test(f)) {
        bm_clear(f);
        free_frames++;
    }
}

void pmm_init(void)
{
    struct mem_region regions[MAX_REGIONS];
    phys_addr_t ram_top = 0;
    int nr = arch_mem_detect(regions, MAX_REGIONS, &ram_top);

    total_frames = ram_top >> PAGE_SHIFT;
    free_frames  = 0;

    /* Place the bitmap right after the kernel image. */
    bitmap = (uint8_t *)PAGE_ALIGN_UP((uint64_t)(uintptr_t)kernel_end);
    uint64_t bitmap_bytes = (total_frames + 7) / 8;

    /* Start fully used, then free what firmware reports as available. */
    for (uint64_t i = 0; i < bitmap_bytes; i++)
        bitmap[i] = 0xFF;

    for (int r = 0; r < nr; r++) {
        uint64_t start = PAGE_ALIGN_UP(regions[r].base) >> PAGE_SHIFT;
        uint64_t stop  = (regions[r].base + regions[r].len) >> PAGE_SHIFT;
        for (uint64_t f = start; f < stop; f++)
            mark_free(f);
    }

    /* Reserve everything below the end of the bitmap: this covers low
     * memory (BIOS/VGA), the kernel image, and the bitmap itself. */
    uint64_t reserved_top = (uint64_t)(uintptr_t)bitmap + bitmap_bytes;
    uint64_t reserved_frames = PAGE_ALIGN_UP(reserved_top) >> PAGE_SHIFT;
    for (uint64_t f = 0; f < reserved_frames; f++)
        mark_used(f);

    alloc_hint = reserved_frames;
}

phys_addr_t pmm_alloc_frame(void)
{
    for (uint64_t i = 0; i < total_frames; i++) {
        uint64_t f = (alloc_hint + i) % total_frames;
        if (!bm_test(f)) {
            bm_set(f);
            free_frames--;
            alloc_hint = f + 1;
            return (phys_addr_t)f << PAGE_SHIFT;
        }
    }
    return 0;                       /* out of memory */
}

void pmm_free_frame(phys_addr_t frame)
{
    mark_free(frame >> PAGE_SHIFT);
}

uint64_t pmm_free_count(void)  { return free_frames; }
uint64_t pmm_total_count(void) { return total_frames; }
