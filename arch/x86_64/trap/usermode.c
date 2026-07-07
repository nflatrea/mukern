/* arch/x86_64/trap/usermode.c - drop the current thread to Ring 3 (M3c).
 *
 * Builds an interrupt-return frame for user privilege and iretq's into it.
 * The TSS RSP0 is pointed at this thread's kernel stack first, so any later
 * trap from Ring 3 (syscall, timer, fault) re-enters the kernel on a valid
 * stack. Does not return: control comes back only via a trap.
 */
#include <stdint.h>
#include "arch/x86_64/trap/gdt.h"
#include "sched.h"

#define USER_CS    (SEL_UCODE | 3)
#define USER_SS    (SEL_UDATA | 3)
#define USER_FLAGS 0x202ull            /* IF set, reserved bit 1 set */

void arch_enter_user(uint64_t entry, uint64_t user_stack_top)
{
    /* Traps from Ring 3 land on this thread's kernel stack. */
    arch_set_kernel_stack(thread_current()->kstack_top);

    __asm__ volatile (
        "pushq %0\n"        /* SS     */
        "pushq %1\n"        /* RSP    */
        "pushq %2\n"        /* RFLAGS */
        "pushq %3\n"        /* CS     */
        "pushq %4\n"        /* RIP    */
        "iretq\n"
        :
        : "r"((uint64_t)USER_SS), "r"(user_stack_top),
          "r"(USER_FLAGS), "r"((uint64_t)USER_CS), "r"(entry)
        : "memory");

    __builtin_unreachable();
}
