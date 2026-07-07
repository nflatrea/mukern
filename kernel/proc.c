/* kernel/proc.c - load flat server binaries and spawn Ring 3 threads (Ph4).
 *
 * Each program is a position-dependent flat binary linked at a fixed user VA
 * (its base). The kernel maps a zeroed image region + a stack at that base,
 * copies the binary in (leaving .bss zero), and entry == base (crt0 first).
 * For this proof of concept all programs share one address space at distinct
 * VAs; per-process address spaces are a later phase.
 *
 * Program bases are supplied by the build (-D..._BASE) so they match the link.
 */
#include "proc.h"
#include "vm.h"
#include "sched.h"
#include "syscall.h"

#define IMG_BYTES   0x10000     /* 64 KiB image (text+data+bss) per program */
#define STK_BYTES   0x4000      /* 16 KiB stack per program                 */
#define STK_OFFSET  0x80000     /* stack top sits this far above the base   */

/* Flat binaries embedded by kernel/rootfs.S. */
extern char init_bin_start[],  init_bin_end[];
extern char pm_bin_start[],    pm_bin_end[];
extern char hello_bin_start[], hello_bin_end[];
extern char vm_bin_start[],    vm_bin_end[];
extern char console_bin_start[], console_bin_end[];
extern char block_bin_start[], block_bin_end[];
extern char fs_bin_start[],    fs_bin_end[];
extern char app_bin_start[],   app_bin_end[];
extern char rogue_bin_start[], rogue_bin_end[];
extern char shell_bin_start[], shell_bin_end[];
extern char shell2_bin_start[], shell2_bin_end[];
extern char vga_bin_start[],   vga_bin_end[];

struct prog {
    char    *start, *end;
    uint64_t base;              /* load VA == entry */
    uint64_t stack_top;
};

static struct prog progs[PROG_COUNT];

static void map_zeroed(uint64_t va, uint64_t bytes)
{
    for (uint64_t off = 0; off < bytes; off += PAGE_SIZE) {
        phys_addr_t f = pmm_alloc_frame();
        vmm_map(va + off, f, VM_PRESENT | VM_WRITE | VM_USER);
        uint8_t *p = (uint8_t *)(va + off);
        for (uint64_t i = 0; i < PAGE_SIZE; i++)
            p[i] = 0;
    }
}

static void load_one(struct prog *pr)
{
    map_zeroed(pr->base, IMG_BYTES);
    map_zeroed(pr->stack_top - STK_BYTES, STK_BYTES);

    uint64_t len = (uint64_t)(pr->end - pr->start);
    uint8_t *dst = (uint8_t *)pr->base;
    for (uint64_t i = 0; i < len; i++)
        dst[i] = (uint8_t)pr->start[i];
}

void rootfs_load_all(void)
{
    progs[PROG_INIT]  = (struct prog){ init_bin_start,  init_bin_end,
                                       INIT_BASE,  INIT_BASE  + STK_OFFSET };
    progs[PROG_PM]    = (struct prog){ pm_bin_start,    pm_bin_end,
                                       PM_BASE,    PM_BASE    + STK_OFFSET };
    progs[PROG_HELLO] = (struct prog){ hello_bin_start, hello_bin_end,
                                       HELLO_BASE, HELLO_BASE + STK_OFFSET };
    progs[PROG_VM]    = (struct prog){ vm_bin_start,    vm_bin_end,
                                       VM_BASE,    VM_BASE    + STK_OFFSET };
    progs[PROG_CONSOLE] = (struct prog){ console_bin_start, console_bin_end,
                                       CON_BASE,   CON_BASE   + STK_OFFSET };
    progs[PROG_BLOCK] = (struct prog){ block_bin_start, block_bin_end,
                                       BLK_BASE,   BLK_BASE   + STK_OFFSET };
    progs[PROG_FS]    = (struct prog){ fs_bin_start,    fs_bin_end,
                                       FS_BASE,    FS_BASE    + STK_OFFSET };
    progs[PROG_APP]   = (struct prog){ app_bin_start,   app_bin_end,
                                       APP_BASE,   APP_BASE   + STK_OFFSET };
    progs[PROG_ROGUE] = (struct prog){ rogue_bin_start, rogue_bin_end,
                                       ROGUE_BASE, ROGUE_BASE + STK_OFFSET };
    progs[PROG_SHELL] = (struct prog){ shell_bin_start, shell_bin_end,
                                       SHELL_BASE, SHELL_BASE + STK_OFFSET };
    progs[PROG_VGA]   = (struct prog){ vga_bin_start,   vga_bin_end,
                                       VGA_BASE,   VGA_BASE   + STK_OFFSET };
    progs[PROG_SHELL2]= (struct prog){ shell2_bin_start, shell2_bin_end,
                                       SHELL2_BASE, SHELL2_BASE + STK_OFFSET };

    for (int i = 0; i < PROG_COUNT; i++)
        load_one(&progs[i]);
}

/* Kernel-side trampoline: runs as a new thread in ring 0, then drops to the
 * program's Ring 3 entry. Defined in arch/x86_64/trap/usermode.c. */
extern void arch_enter_user(uint64_t entry, uint64_t user_stack_top);

static void user_bootstrap(void *arg)
{
    struct prog *pr = (struct prog *)arg;
    arch_enter_user(pr->base, pr->stack_top);   /* never returns */
}

int proc_spawn(int prog)
{
    if (prog < 0 || prog >= PROG_COUNT)
        return -1;
    return thread_create(user_bootstrap, &progs[prog]);
}

void proc_start_init(void)
{
    proc_spawn(PROG_INIT);
}
