/* arch/x86_64/trap/isr.S  (GENERATED - see Makefile/commit notes)
 *
 * ISR stubs for vectors 0..47. Each stub normalises the stack to
 * [..CPU frame.., err, vec] then jumps to isr_common, which saves the
 * general-purpose registers, calls isr_handler(struct trapframe*), and
 * returns with iretq. isr_stub_table[] lets idt.c install them in a loop.
 */
.code64
.section .text

.global isr_0
.type isr_0, @function
isr_0:
    pushq $0             /* dummy error code            */
    pushq $0
    jmp isr_common

.global isr_1
.type isr_1, @function
isr_1:
    pushq $0             /* dummy error code            */
    pushq $1
    jmp isr_common

.global isr_2
.type isr_2, @function
isr_2:
    pushq $0             /* dummy error code            */
    pushq $2
    jmp isr_common

.global isr_3
.type isr_3, @function
isr_3:
    pushq $0             /* dummy error code            */
    pushq $3
    jmp isr_common

.global isr_4
.type isr_4, @function
isr_4:
    pushq $0             /* dummy error code            */
    pushq $4
    jmp isr_common

.global isr_5
.type isr_5, @function
isr_5:
    pushq $0             /* dummy error code            */
    pushq $5
    jmp isr_common

.global isr_6
.type isr_6, @function
isr_6:
    pushq $0             /* dummy error code            */
    pushq $6
    jmp isr_common

.global isr_7
.type isr_7, @function
isr_7:
    pushq $0             /* dummy error code            */
    pushq $7
    jmp isr_common

.global isr_8
.type isr_8, @function
isr_8:
    pushq $8            /* CPU already pushed err code */
    jmp isr_common

.global isr_9
.type isr_9, @function
isr_9:
    pushq $0             /* dummy error code            */
    pushq $9
    jmp isr_common

.global isr_10
.type isr_10, @function
isr_10:
    pushq $10            /* CPU already pushed err code */
    jmp isr_common

.global isr_11
.type isr_11, @function
isr_11:
    pushq $11            /* CPU already pushed err code */
    jmp isr_common

.global isr_12
.type isr_12, @function
isr_12:
    pushq $12            /* CPU already pushed err code */
    jmp isr_common

.global isr_13
.type isr_13, @function
isr_13:
    pushq $13            /* CPU already pushed err code */
    jmp isr_common

.global isr_14
.type isr_14, @function
isr_14:
    pushq $14            /* CPU already pushed err code */
    jmp isr_common

.global isr_15
.type isr_15, @function
isr_15:
    pushq $0             /* dummy error code            */
    pushq $15
    jmp isr_common

.global isr_16
.type isr_16, @function
isr_16:
    pushq $0             /* dummy error code            */
    pushq $16
    jmp isr_common

.global isr_17
.type isr_17, @function
isr_17:
    pushq $17            /* CPU already pushed err code */
    jmp isr_common

.global isr_18
.type isr_18, @function
isr_18:
    pushq $0             /* dummy error code            */
    pushq $18
    jmp isr_common

.global isr_19
.type isr_19, @function
isr_19:
    pushq $0             /* dummy error code            */
    pushq $19
    jmp isr_common

.global isr_20
.type isr_20, @function
isr_20:
    pushq $0             /* dummy error code            */
    pushq $20
    jmp isr_common

.global isr_21
.type isr_21, @function
isr_21:
    pushq $21            /* CPU already pushed err code */
    jmp isr_common

.global isr_22
.type isr_22, @function
isr_22:
    pushq $0             /* dummy error code            */
    pushq $22
    jmp isr_common

.global isr_23
.type isr_23, @function
isr_23:
    pushq $0             /* dummy error code            */
    pushq $23
    jmp isr_common

.global isr_24
.type isr_24, @function
isr_24:
    pushq $0             /* dummy error code            */
    pushq $24
    jmp isr_common

.global isr_25
.type isr_25, @function
isr_25:
    pushq $0             /* dummy error code            */
    pushq $25
    jmp isr_common

.global isr_26
.type isr_26, @function
isr_26:
    pushq $0             /* dummy error code            */
    pushq $26
    jmp isr_common

.global isr_27
.type isr_27, @function
isr_27:
    pushq $0             /* dummy error code            */
    pushq $27
    jmp isr_common

.global isr_28
.type isr_28, @function
isr_28:
    pushq $0             /* dummy error code            */
    pushq $28
    jmp isr_common

.global isr_29
.type isr_29, @function
isr_29:
    pushq $29            /* CPU already pushed err code */
    jmp isr_common

.global isr_30
.type isr_30, @function
isr_30:
    pushq $30            /* CPU already pushed err code */
    jmp isr_common

.global isr_31
.type isr_31, @function
isr_31:
    pushq $0             /* dummy error code            */
    pushq $31
    jmp isr_common

.global isr_32
.type isr_32, @function
isr_32:
    pushq $0             /* dummy error code            */
    pushq $32
    jmp isr_common

.global isr_33
.type isr_33, @function
isr_33:
    pushq $0             /* dummy error code            */
    pushq $33
    jmp isr_common

.global isr_34
.type isr_34, @function
isr_34:
    pushq $0             /* dummy error code            */
    pushq $34
    jmp isr_common

.global isr_35
.type isr_35, @function
isr_35:
    pushq $0             /* dummy error code            */
    pushq $35
    jmp isr_common

.global isr_36
.type isr_36, @function
isr_36:
    pushq $0             /* dummy error code            */
    pushq $36
    jmp isr_common

.global isr_37
.type isr_37, @function
isr_37:
    pushq $0             /* dummy error code            */
    pushq $37
    jmp isr_common

.global isr_38
.type isr_38, @function
isr_38:
    pushq $0             /* dummy error code            */
    pushq $38
    jmp isr_common

.global isr_39
.type isr_39, @function
isr_39:
    pushq $0             /* dummy error code            */
    pushq $39
    jmp isr_common

.global isr_40
.type isr_40, @function
isr_40:
    pushq $0             /* dummy error code            */
    pushq $40
    jmp isr_common

.global isr_41
.type isr_41, @function
isr_41:
    pushq $0             /* dummy error code            */
    pushq $41
    jmp isr_common

.global isr_42
.type isr_42, @function
isr_42:
    pushq $0             /* dummy error code            */
    pushq $42
    jmp isr_common

.global isr_43
.type isr_43, @function
isr_43:
    pushq $0             /* dummy error code            */
    pushq $43
    jmp isr_common

.global isr_44
.type isr_44, @function
isr_44:
    pushq $0             /* dummy error code            */
    pushq $44
    jmp isr_common

.global isr_45
.type isr_45, @function
isr_45:
    pushq $0             /* dummy error code            */
    pushq $45
    jmp isr_common

.global isr_46
.type isr_46, @function
isr_46:
    pushq $0             /* dummy error code            */
    pushq $46
    jmp isr_common

.global isr_47
.type isr_47, @function
isr_47:
    pushq $0             /* dummy error code            */
    pushq $47
    jmp isr_common

isr_common:
    /* Save GP registers (order mirrors struct trapframe). */
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, %rdi          /* arg0 = &trapframe */
    call isr_handler

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16, %rsp           /* discard vec + err */
    iretq

/* Table of stub entry points, indexed by vector number. */
.section .rodata
.align 8
.global isr_stub_table
isr_stub_table:
    .quad isr_0
    .quad isr_1
    .quad isr_2
    .quad isr_3
    .quad isr_4
    .quad isr_5
    .quad isr_6
    .quad isr_7
    .quad isr_8
    .quad isr_9
    .quad isr_10
    .quad isr_11
    .quad isr_12
    .quad isr_13
    .quad isr_14
    .quad isr_15
    .quad isr_16
    .quad isr_17
    .quad isr_18
    .quad isr_19
    .quad isr_20
    .quad isr_21
    .quad isr_22
    .quad isr_23
    .quad isr_24
    .quad isr_25
    .quad isr_26
    .quad isr_27
    .quad isr_28
    .quad isr_29
    .quad isr_30
    .quad isr_31
    .quad isr_32
    .quad isr_33
    .quad isr_34
    .quad isr_35
    .quad isr_36
    .quad isr_37
    .quad isr_38
    .quad isr_39
    .quad isr_40
    .quad isr_41
    .quad isr_42
    .quad isr_43
    .quad isr_44
    .quad isr_45
    .quad isr_46
    .quad isr_47

/* ---- M3c: software syscall entry (int 0x80). Same frame shape as the
 * generated stubs (no CPU error code, so push a dummy), reusing isr_common
 * so the dispatcher sees a normal struct trapframe. ---- */
.global isr_syscall
.type isr_syscall, @function
isr_syscall:
    pushq $0                 /* dummy error code */
    pushq $0x80              /* vector */
    jmp isr_common
