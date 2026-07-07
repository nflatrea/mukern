/* arch/x86_64/trap/trap.c - interrupt demux + syscall + fault forwarding.
 *
 * Mechanism, not policy:
 *  - IRQ0 (timer): acknowledge, run the tick callback (drives the scheduler).
 *  - int 0x80 (syscall): the user/kernel ABI (Phase 3 + Phase 4 calls).
 *  - #PF from Ring 3: forward to the registered user pager via IPC (M4c);
 *    the kernel holds no paging policy. Faults with no pager, or from the
 *    kernel, panic.
 */
#include <stdint.h>
#include "arch/x86_64/trap/idt.h"
#include "arch/x86_64/trap/gdt.h"
#include "arch/x86_64/hal/pic.h"
#include "arch/x86_64/hal/pit.h"
#include "arch/x86_64/hal/io.h"
#include "hal.h"
#include "vm.h"
#include "sched.h"
#include "ipc.h"
#include "cap.h"
#include "proc.h"
#include "syscall.h"

#define VEC_IRQ_BASE 32
#define VEC_TIMER    (VEC_IRQ_BASE + 0)
#define VEC_PF       14

static void (*g_on_tick)(void);
static int  g_pager_tid = -1;           /* user thread registered as pager */
static volatile uint64_t g_ticks;       /* timer ticks since boot          */
static int      g_irq_tid[16];          /* per-IRQ registered driver tid    */
static uint16_t g_irq_mask = 0xFFFF;    /* shadow of the PIC mask           */

static void puts_raw(const char *s) { while (*s) hal_console_putc(*s++); }

static void put_dec(uint64_t v)
{
    char buf[21];
    int i = 0;
    if (v == 0) { hal_console_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i) hal_console_putc(buf[--i]);
}

static void put_hex(uint64_t v)
{
    static const char d[] = "0123456789abcdef";
    puts_raw("0x");
    for (int s = 60; s >= 0; s -= 4)
        hal_console_putc(d[(v >> s) & 0xF]);
}

static void panic_exception(struct trapframe *tf)
{
    puts_raw("\n[mukern] EXCEPTION vec=");
    put_dec(tf->vec);
    puts_raw(" err=");
    put_dec(tf->err);
    puts_raw(" rip=");
    put_hex(tf->rip);
    hal_console_putc('\n');
    for (;;) { hal_irq_disable(); hal_halt(); }
}

/* May `self` send to `dest`? Root and the init endpoint are always reachable;
 * otherwise the sender must hold an endpoint capability, or be replying to a
 * thread that is specifically blocked waiting to receive from it. */
static int ipc_send_allowed(struct thread *self, int dest)
{
    if (self->id == 0 || dest == 0)
        return 1;
    if (cap_has_endpoint(self, dest))
        return 1;
    struct thread *d = thread_by_id(dest);
    if (d && d->ipc_call_to == self->id)
        return 1;                               /* dest has an open call to us */
    if (d && d->state == T_BLOCKED && d->ipc_state == IPC_RECEIVING &&
        d->ipc_peer == self->id)
        return 1;                               /* kernel-mediated reply (pager) */
    return 0;
}

static void do_syscall(struct trapframe *tf)
{
    struct thread *self = thread_current();
    switch (tf->rax) {
    case SYS_PUTC:
        hal_console_putc((char)tf->rdi);
        tf->rax = 0;
        break;
    case SYS_EXIT:
        self->state = T_DEAD;
        schedule();                     /* IF=0; never returns to this thread */
        break;
    case SYS_GETTID:
        tf->rax = (uint64_t)self->id;
        break;
    case SYS_SEND: {
        if (!ipc_send_allowed(self, (int)tf->rdi)) { tf->rax = (uint64_t)-1; break; }
        self->ipc_call_to = (int)tf->rdi;          /* authorise a reply (race-free) */
        struct ipc_msg m = *(struct ipc_msg *)tf->rsi;          /* read user */
        tf->rax = (uint64_t)ipc_send_locked(self, (int)tf->rdi, &m);
        break;
    }
    case SYS_RECV: {
        struct ipc_msg m;
        int from = ipc_recv_locked(self, (int)tf->rdi, &m);
        *(struct ipc_msg *)tf->rsi = m;                         /* write user */
        if (from == self->ipc_call_to) self->ipc_call_to = -1;  /* call completed */
        tf->rax = (uint64_t)(int64_t)from;
        break;
    }
    case SYS_SPAWN: {
        int child = proc_spawn((int)tf->rdi);
        if (child >= 0)
            cap_grant_endpoint(self, child);    /* parent may message its child */
        tf->rax = (uint64_t)(int64_t)child;
        break;
    }
    case SYS_MAP: {
        phys_addr_t f = pmm_alloc_frame();
        if (f && vmm_map(PAGE_ALIGN_DOWN(tf->rdi), f,
                         VM_PRESENT | VM_WRITE | VM_USER) == 0)
            tf->rax = 0;
        else
            tf->rax = (uint64_t)-1;
        break;
    }
    case SYS_SET_PAGER:
        g_pager_tid = self->id;
        tf->rax = 0;
        break;
    case SYS_OUTB:
        if (!cap_has_port(self, (uint16_t)tf->rdi)) { tf->rax = (uint64_t)-1; break; }
        outb((uint16_t)tf->rdi, (uint8_t)tf->rsi);
        tf->rax = 0;
        break;
    case SYS_INB:
        if (!cap_has_port(self, (uint16_t)tf->rdi)) { tf->rax = (uint64_t)-1; break; }
        tf->rax = (uint64_t)inb((uint16_t)tf->rdi);
        break;
    case SYS_INW:
        if (!cap_has_port(self, (uint16_t)tf->rdi)) { tf->rax = (uint64_t)-1; break; }
        tf->rax = (uint64_t)inw((uint16_t)tf->rdi);
        break;
    case SYS_GRANT_IO:
        /* Only the root task (init, tid 0) may hand out hardware. */
        if (self->id != 0) { tf->rax = (uint64_t)-1; break; }
        tf->rax = (uint64_t)(int64_t)cap_grant_io(thread_by_id((int)tf->rdi),
                                                  (uint16_t)tf->rsi,
                                                  (uint16_t)tf->rdx);
        break;
    case SYS_UPTIME:
        tf->rax = g_ticks;
        break;
    case SYS_MEMINFO:
        tf->rax = tf->rdi ? pmm_total_count() : pmm_free_count();
        break;
    case SYS_TINFO: {
        struct thread *t = thread_by_id((int)tf->rdi);
        tf->rax = t ? (uint64_t)(int64_t)t->state : (uint64_t)-1;
        break;
    }
    case SYS_IRQ_REGISTER: {
        int irq = (int)tf->rdi;
        if (irq < 0 || irq >= 16) { tf->rax = (uint64_t)-1; break; }
        g_irq_tid[irq] = self->id;
        g_irq_mask &= (uint16_t)~(1u << irq);   /* unmask this line */
        pic_set_mask(g_irq_mask);
        tf->rax = 0;
        break;
    }
    case SYS_MAP_PHYS:
        /* Map device/physical memory into the user address space. Only the
         * root task may hand out a device region (like SYS_GRANT_IO). */
        if (self->id != 0) { tf->rax = (uint64_t)-1; break; }
        tf->rax = (uint64_t)(int64_t)vmm_map(tf->rdi, tf->rsi,
                                             VM_PRESENT | VM_WRITE | VM_USER);
        break;
    case SYS_GRANT_CAP: {
        /* Delegate a send-capability for endpoint rsi to thread rdi. The caller
         * must itself hold that endpoint (or be root). */
        int ep = (int)tf->rsi;
        if (self->id != 0 && !cap_has_endpoint(self, ep)) { tf->rax = (uint64_t)-1; break; }
        tf->rax = (uint64_t)(int64_t)cap_grant_endpoint(thread_by_id((int)tf->rdi), ep);
        break;
    }
    default:
        tf->rax = (uint64_t)-1;
        break;
    }
}

void isr_handler(struct trapframe *tf)
{
    if (tf->vec == VEC_TIMER) {
        g_ticks++;
        pic_send_eoi(0);                /* EOI before any context switch */
        if (g_on_tick)
            g_on_tick();
        return;
    }

    if (tf->vec == SYSCALL_VECTOR) {
        do_syscall(tf);
        return;
    }

    if (tf->vec == VEC_PF) {
        uint64_t addr = arch_fault_addr();
        struct thread *self = thread_current();
        int from_user = (tf->cs & 3) == 3;

        if (from_user && g_pager_tid >= 0 && self->id != g_pager_tid) {
            /* M4c: forward the fault to the user pager and wait for its verdict
             * (a send/recv "call"). reply.data0 == 0 means the page was mapped;
             * non-zero is a policy decision to kill the faulting thread (H2). */
            struct ipc_msg fault = {
                .label = MSG_FAULT,
                .data0 = (uint64_t)self->id,
                .data1 = addr,
            };
            struct ipc_msg reply;
            ipc_send_locked(self, g_pager_tid, &fault);
            ipc_recv_locked(self, g_pager_tid, &reply);
            if (reply.data0 != 0) {         /* pager says: kill this process */
                self->state = T_DEAD;
                schedule();                 /* torn down; never returns */
            }
            return;                         /* page mapped; retry instruction */
        }

        puts_raw("\n[mukern] PAGE FAULT @ ");
        put_hex(addr);
        panic_exception(tf);
    }

    if (tf->vec < VEC_IRQ_BASE)
        panic_exception(tf);

    /* Hardware IRQ (not the timer): notify the registered driver, if any, by
     * bumping its pending count and waking it if it is blocked receiving. The
     * driver picks this up as a MSG_IRQ from ipc_recv. */
    {
        int irq = (int)tf->vec - VEC_IRQ_BASE;
        if (irq >= 0 && irq < 16) {
            int tid = g_irq_tid[irq];
            if (tid >= 0) {
                struct thread *d = thread_by_id(tid);
                if (d && d->state != T_UNUSED && d->state != T_DEAD) {
                    d->irq_pending++;
                    if (d->state == T_BLOCKED && d->ipc_state == IPC_RECEIVING)
                        thread_wake(d);
                }
            }
            pic_send_eoi((uint8_t)irq);
        }
    }
}

/* ---- HAL surface used by the kernel ---- */
void hal_traps_init(void)
{
    gdt_init();                         /* runtime GDT + TSS (M3c) */
    pic_remap(VEC_IRQ_BASE, VEC_IRQ_BASE + 8);
    for (int i = 0; i < 16; i++)
        g_irq_tid[i] = -1;
    g_irq_mask = (uint16_t)~(1u << 0);  /* unmask IRQ0 (timer) only */
    pic_set_mask(g_irq_mask);
    idt_init();
}

void hal_timer_init(uint32_t hz, void (*on_tick)(void))
{
    g_on_tick = on_tick;
    pit_init(hz);
}
