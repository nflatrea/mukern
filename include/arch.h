/*
file: arch.h
desc: the portable kernel's view of each architecture.
      Grows one milestone at a time. The core calls only these (plus the 
      console HAL in console.h); never arch-internal symbols.
*/
#ifndef MUKERN_ARCH_H
#define MUKERN_ARCH_H

/* M1: install trap/exception vectors (IDT / VBAR_EL1 / stvec). */
void arch_traps_init(void);

/* M1 demo hook: deliberately raise a synchronous CPU exception so the
 * trap path can be exercised from the REPL. */
void arch_trigger_fault(void);

#endif /* MUKERN_ARCH_H */
