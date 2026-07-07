/* arch/x86_64/trap/switch.S - kernel-thread context switch (M2c).
 *
 *   context_switch(uint64_t *old_rsp, uint64_t new_rsp)   [rdi, rsi]
 *
 * Saves the callee-saved registers of the current thread onto its own stack,
 * stores the resulting rsp into *old_rsp, loads new_rsp, restores the next
 * thread's callee-saved registers and returns into it. A freshly created
 * thread is given a hand-crafted stack so this `ret` lands in
 * thread_trampoline with r12=entry, r13=arg.
 */
.code64
.section .text

.global context_switch
.type context_switch, @function
context_switch:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, (%rdi)        /* *old_rsp = rsp */
    movq %rsi, %rsp          /* rsp = new_rsp  */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
.size context_switch, . - context_switch

/* Entry shim for new threads. context_switch's `ret` lands here with the
 * entry function in r12 and its argument in r13 (planted by thread_create). */
.global thread_trampoline
.type thread_trampoline, @function
thread_trampoline:
    sti                      /* run the new thread with interrupts on */
    movq %r13, %rdi          /* arg */
    call *%r12               /* entry(arg) */
    call thread_exit         /* if it ever returns */
.Lhang_tramp:
    hlt
    jmp .Lhang_tramp
.size thread_trampoline, . - thread_trampoline
