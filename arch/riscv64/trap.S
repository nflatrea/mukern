/*
arch/riscv64/trap.S

S-mode trap vector (direct mode).
Saves all GPRs + sepc/scause/stval, calls riscv_trap(struct rregs *).
Frame layout matches struct rregs in trap.c.
*/

.section .text
.balign 4
.global trap_vector
trap_vector:
    addi sp, sp, -288
    sd x1,  8(sp)
    sd x3,  24(sp)
    sd x4,  32(sp)
    sd x5,  40(sp)
    sd x6,  48(sp)
    sd x7,  56(sp)
    sd x8,  64(sp)
    sd x9,  72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    addi t0, sp, 288        /* original sp (x5/t0 already saved) */
    sd t0, 16(sp)           /* x2 slot */
    csrr t0, sepc
    sd t0, 256(sp)
    csrr t0, scause
    sd t0, 264(sp)
    csrr t0, stval
    sd t0, 272(sp)
    mv a0, sp
    call riscv_trap
1:  wfi
    j 1b
