/* lib/crt0.S - user program entry. The kernel iretq's here with a fresh,
 * 16-byte aligned user stack; we just call main() and exit with its result. */
#include "syscall.h"
.code64
.section .text.start
.global _ustart
.type _ustart, @function
_ustart:
    xorq %rbp, %rbp
    call main
    movl %eax, %edi          /* exit code = main() return value */
    movq $SYS_EXIT, %rax
    int  $0x80
1:  jmp 1b
