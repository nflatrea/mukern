/*
arch/arm64/trap.S

EL1 exception vectors.
All 16 entries funnel into one save path (every trap is fatal in M1).
Frame layout matches struct aregs in trap.c.
*/

.macro VEC label
.balign 0x80
    b \label
.endm

.section .text
.balign 0x800
.global vectors
vectors:
    VEC trap_entry      /* EL1t sync   */
    VEC trap_entry      /* EL1t irq    */
    VEC trap_entry      /* EL1t fiq    */
    VEC trap_entry      /* EL1t serror */
    VEC trap_entry      /* EL1h sync   */
    VEC trap_entry      /* EL1h irq    */
    VEC trap_entry      /* EL1h fiq    */
    VEC trap_entry      /* EL1h serror */
    VEC trap_entry      /* EL0_64 sync */
    VEC trap_entry      /* EL0_64 irq  */
    VEC trap_entry      /* EL0_64 fiq  */
    VEC trap_entry      /* EL0_64 serr */
    VEC trap_entry      /* EL0_32 sync */
    VEC trap_entry      /* EL0_32 irq  */
    VEC trap_entry      /* EL0_32 fiq  */
    VEC trap_entry      /* EL0_32 serr */

trap_entry:
    sub  sp, sp, #288
    stp  x0,  x1,  [sp, #0]
    stp  x2,  x3,  [sp, #16]
    stp  x4,  x5,  [sp, #32]
    stp  x6,  x7,  [sp, #48]
    stp  x8,  x9,  [sp, #64]
    stp  x10, x11, [sp, #80]
    stp  x12, x13, [sp, #96]
    stp  x14, x15, [sp, #112]
    stp  x16, x17, [sp, #128]
    stp  x18, x19, [sp, #144]
    stp  x20, x21, [sp, #160]
    stp  x22, x23, [sp, #176]
    stp  x24, x25, [sp, #192]
    stp  x26, x27, [sp, #208]
    stp  x28, x29, [sp, #224]
    str  x30,      [sp, #240]
    mrs  x0, elr_el1
    mrs  x1, spsr_el1
    stp  x0, x1,   [sp, #248]
    mrs  x0, esr_el1
    mrs  x1, far_el1
    stp  x0, x1,   [sp, #264]
    mov  x0, sp
    bl   arm_trap
1:  wfe
    b    1b
