/* include/types.h - shared kernel types. */
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint64_t phys_addr_t;   /* physical address / frame address */
typedef uint64_t virt_addr_t;   /* virtual address                  */

#define PAGE_SIZE   4096u
#define PAGE_SHIFT  12
#define PAGE_MASK   (PAGE_SIZE - 1)

#define PAGE_ALIGN_DOWN(x) ((x) & ~(uint64_t)PAGE_MASK)
#define PAGE_ALIGN_UP(x)   (((x) + PAGE_MASK) & ~(uint64_t)PAGE_MASK)
