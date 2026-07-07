/* arch/riscv64/trap/switch.S - kernel-thread context switch.
 *
 *   context_switch(uint64_t *old_sp, uint64_t new_sp)    [a0, a1]
 *
 * Saves callee-saved registers (ra, s0-s11) on the current stack, stores the
 * resulting sp into *old_sp, loads new_sp, restores, and returns into the next
 * thread. A new thread's stack is planted by arch_init_stack so this return
 * lands in thread_trampoline with s1=entry, s2=arg.
 */
.section .text
.global context_switch
context_switch:
    addi    sp, sp, -112
    sd      ra,  0(sp)
    sd      s0,  8(sp)
    sd      s1, 16(sp)
    sd      s2, 24(sp)
    sd      s3, 32(sp)
    sd      s4, 40(sp)
    sd      s5, 48(sp)
    sd      s6, 56(sp)
    sd      s7, 64(sp)
    sd      s8, 72(sp)
    sd      s9, 80(sp)
    sd      s10,88(sp)
    sd      s11,96(sp)
    sd      sp, 0(a0)               /* *old_sp = sp */
    mv      sp, a1                  /* sp = new_sp  */
    ld      ra,  0(sp)
    ld      s0,  8(sp)
    ld      s1, 16(sp)
    ld      s2, 24(sp)
    ld      s3, 32(sp)
    ld      s4, 40(sp)
    ld      s5, 48(sp)
    ld      s6, 56(sp)
    ld      s7, 64(sp)
    ld      s8, 72(sp)
    ld      s9, 80(sp)
    ld      s10,88(sp)
    ld      s11,96(sp)
    addi    sp, sp, 112
    ret

/* New-thread entry shim: reached via context_switch's `ret`, with the entry
 * function in s1 and its argument in s2 (planted by arch_init_stack). */
.global thread_trampoline
thread_trampoline:
    csrsi   sstatus, 2              /* enable interrupts (SIE) for this thread */
    mv      a0, s2                  /* arg */
    jalr    s1                      /* entry(arg) */
    call    thread_exit
1:  wfi
    j       1b
