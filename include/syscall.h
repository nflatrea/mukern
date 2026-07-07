/* include/syscall.h - system call numbers + program ids (kernel/user ABI).
 *
 * Convention (int 0x80): rax = syscall number, rdi/rsi/rdx/rcx = args,
 * return value in rax. IPC messages are passed by pointer to a user-space
 * struct ipc_msg (the kernel may read/write user memory directly).
 */
#pragma once

/* ---- system calls ---- */
#define SYS_EXIT       0   /* rdi = exit code                              */
#define SYS_PUTC       1   /* rdi = character (kernel boot/panic console)  */
#define SYS_SEND       2   /* rdi = dest tid, rsi = *ipc_msg  -> rax 0/-1  */
#define SYS_RECV       3   /* rdi = src tid/-1, rsi = *ipc_msg -> rax sender*/
#define SYS_SPAWN      4   /* rdi = program id -> rax = new tid or -1       */
#define SYS_MAP        5   /* rdi = vaddr -> rax 0/-1 (alloc+map a frame)   */
#define SYS_SET_PAGER  6   /* register caller as the page-fault server      */
#define SYS_GETTID     7   /* -> rax = caller tid                          */
#define SYS_OUTB       8   /* rdi = port, rsi = byte    (driver port I/O)   */
#define SYS_INB        9   /* rdi = port -> rax = byte                      */
#define SYS_INW       10   /* rdi = port -> rax = word (16-bit, ATA data)   */
#define SYS_GRANT_IO  11   /* rdi = tid, rsi = port base, rdx = len (tid 0) */
#define SYS_UPTIME    12   /* -> rax = timer ticks since boot               */
#define SYS_MEMINFO   13   /* rdi=0 -> free frames; rdi=1 -> total frames    */
#define SYS_TINFO     14   /* rdi = tid -> rax = thread state (-1 if none)   */
#define SYS_IRQ_REGISTER 15 /* rdi = irq -> register caller; unmask line    */
#define SYS_MAP_PHYS  16   /* rdi = vaddr, rsi = paddr (tid 0): map device  */
#define SYS_GRANT_CAP 17   /* rdi = tid, rsi = endpoint tid: delegate send  */

/* ---- programs in the boot rootfs (indices into the kernel's table) ---- */
#define PROG_INIT      0
#define PROG_PM        1
#define PROG_HELLO     2
#define PROG_VM        3
#define PROG_CONSOLE   4
#define PROG_BLOCK     5
#define PROG_FS        6
#define PROG_APP       7
#define PROG_ROGUE     8
#define PROG_SHELL     9
#define PROG_VGA       10
#define PROG_SHELL2    11
#define PROG_COUNT     12

/* ---- well-known thread ids (init spawns this infrastructure in order) -- */
#define CON_TID        1   /* console driver  */
#define BLK_TID        2   /* block driver    */
#define VM_TID         3   /* pager           */
#define PM_TID         4   /* process manager */
#define FS_TID         5   /* filesystem      */
#define VGA_TID        6   /* VGA display     */

/* VA where init maps the VGA text buffer (phys 0xB8000) for the vga server */
#define VGA_FB_VA      0x0000400000b00000ull
