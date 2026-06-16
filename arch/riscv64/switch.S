/*
arch/riscv64/switch.S

Cooperative context switch for RV64. Saves ra + s0..s11 onto the task stack;
the saved sp is the handle. The final `ret` (jr ra) resumes at the restored
ra. task_trampoline is planted as the initial ra so the first switch lands in
entry(arg).
*/
.text

/* void arch_switch(void **save_sp /a0/, void *load_sp /a1/) */
.global arch_switch
arch_switch:
    addi sp, sp, -112
    sd ra,  0(sp)
    sd s0,  8(sp)
    sd s1,  16(sp)
    sd s2,  24(sp)
    sd s3,  32(sp)
    sd s4,  40(sp)
    sd s5,  48(sp)
    sd s6,  56(sp)
    sd s7,  64(sp)
    sd s8,  72(sp)
    sd s9,  80(sp)
    sd s10, 88(sp)
    sd s11, 96(sp)
    mv   t0, sp
    sd   t0, 0(a0)         /* *save_sp = sp */
    mv   sp, a1           /* sp = load_sp  */
    ld ra,  0(sp)
    ld s0,  8(sp)
    ld s1,  16(sp)
    ld s2,  24(sp)
    ld s3,  32(sp)
    ld s4,  40(sp)
    ld s5,  48(sp)
    ld s6,  56(sp)
    ld s7,  64(sp)
    ld s8,  72(sp)
    ld s9,  80(sp)
    ld s10, 88(sp)
    ld s11, 96(sp)
    addi sp, sp, 112
    ret

/* entry in s0, arg in s1 (planted by arch_ctx_init). */
.global task_trampoline
task_trampoline:
    mv a0, s1
    jalr s0
    call task_exit
1:  wfi
    j 1b
