/*
arch/arm64/switch.S

Cooperative context switch for AArch64. Saves x19..x30 (callee-saved + fp/lr)
onto the task's stack; the saved sp is the handle. The final `ret` resumes at
the restored x30. task_trampoline is planted as the initial x30 so the first
switch lands in entry(arg).
*/
.text

/* void arch_switch(void **save_sp /x0/, void *load_sp /x1/) */
.global arch_switch
arch_switch:
    stp x19, x20, [sp, #-96]!
    stp x21, x22, [sp, #16]
    stp x23, x24, [sp, #32]
    stp x25, x26, [sp, #48]
    stp x27, x28, [sp, #64]
    stp x29, x30, [sp, #80]
    mov x2, sp
    str x2, [x0]            /* *save_sp = sp */
    mov sp, x1             /* sp = load_sp  */
    ldp x21, x22, [sp, #16]
    ldp x23, x24, [sp, #32]
    ldp x25, x26, [sp, #48]
    ldp x27, x28, [sp, #64]
    ldp x29, x30, [sp, #80]
    ldp x19, x20, [sp], #96
    ret

/* entry in x19, arg in x20 (planted by arch_ctx_init). */
.global task_trampoline
task_trampoline:
    mov x0, x20
    blr x19
    bl  task_exit
1:  wfe
    b 1b
