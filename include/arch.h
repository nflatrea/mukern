/*
file: arch.h
desc: The portable kernel's view of the CPU. The core (kernel/core/) calls
      only these arch primitives plus the device HAL in hal.h; never any
      arch-internal symbol.

      Microkernel note: the kernel proper knows about exactly three things --
      tasks, scheduling and IPC. Everything here is the small amount of
      CPU-specific glue needed to make those three portable across
      x86_64 / arm64 / riscv64. Devices live OUTSIDE the kernel, behind the
      IPC protocol in con.h, implemented by driver server tasks.
*/
#ifndef MUKERN_ARCH_H
#define MUKERN_ARCH_H

#include <stdint.h>

/* --- traps / exceptions (M1) ------------------------------------------- */
void arch_traps_init(void);     /* install IDT / VBAR_EL1 / stvec           */
void arch_trigger_fault(void);  /* REPL demo: raise a synchronous exception */

/* --- cpu lifecycle ----------------------------------------------------- */
const char *arch_name(void);                           /* arch string       */
void        arch_halt(void) __attribute__((noreturn)); /* park CPU forever  */
void        arch_relax(void);                          /* idle hint         */

/* --- context switching (M2) -------------------------------------------- *
 * A task's saved machine context is just its stack pointer; all callee-
 * saved registers live on that stack. arch_switch() saves the current
 * context into *save_sp and resumes the context at load_sp. arch_ctx_init()
 * forges a fresh stack so the first switch into a new task "returns" into
 * trampoline(entry, arg).                                                  */
void  arch_switch(void **save_sp, void *load_sp);
void *arch_ctx_init(void *stack_top, void (*entry)(void *), void *arg);

#endif /* MUKERN_ARCH_H */
