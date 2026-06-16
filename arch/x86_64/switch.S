/*
arch/x86_64/switch.S

Cooperative context switch. A task's whole machine context is the six
SysV callee-saved registers pushed onto its own stack; the saved stack
pointer is the handle. arch_switch() spills them, parks %rsp into *save_sp,
loads the new %rsp and unspills -- the `ret` then resumes the new task
wherever it last called arch_switch().

task_trampoline is the synthetic return address arch_ctx_init() plants on a
brand-new stack, so the first switch into a task lands in entry(arg).
*/
.text

/* void arch_switch(void **save_sp /rdi/, void *load_sp /rsi/) */
.global arch_switch
arch_switch:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq  %rsp, (%rdi)      /* *save_sp = rsp                  */
    movq  %rsi, %rsp        /* rsp = load_sp                   */
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    ret

/* First-run entry. arch_ctx_init() leaves entry in r12 and arg in r13. */
.global task_trampoline
task_trampoline:
    movq  %r13, %rdi       /* arg          */
    callq *%r12            /* entry(arg)   */
    callq task_exit        /* if it returns, retire the task */
1:  hlt
    jmp 1b
