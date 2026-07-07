/* lib/usys.h - user-space syscall stubs + tiny helpers (header-only).
 *
 * Ring 3 code links against no libc; this is the entire user runtime surface.
 * Each call is `int 0x80` with rax=number, rdi/rsi=args, result in rax.
 */
#pragma once
#include "syscall.h"
#include "ipc.h"

static inline long __sc(long n, long a, long b)
{
    long r;
    __asm__ volatile ("int $0x80"
                      : "=a"(r)
                      : "a"(n), "D"(a), "S"(b)
                      : "memory");
    return r;
}

static inline void u_exit(long code)        { __sc(SYS_EXIT, code, 0); for(;;){} }
static inline void u_putc(char c)           { __sc(SYS_PUTC, (unsigned char)c, 0); }
static inline long u_send(long dst, struct ipc_msg *m) { return __sc(SYS_SEND, dst, (long)m); }
static inline long u_recv(long src, struct ipc_msg *m) { return __sc(SYS_RECV, src, (long)m); }
static inline long u_spawn(long prog)       { return __sc(SYS_SPAWN, prog, 0); }
static inline long u_map(long vaddr)        { return __sc(SYS_MAP, vaddr, 0); }
static inline long u_set_pager(void)        { return __sc(SYS_SET_PAGER, 0, 0); }
static inline long u_gettid(void)           { return __sc(SYS_GETTID, 0, 0); }
static inline unsigned long u_uptime(void)  { return (unsigned long)__sc(SYS_UPTIME, 0, 0); }
static inline unsigned long u_meminfo(long w){ return (unsigned long)__sc(SYS_MEMINFO, w, 0); }
static inline long u_tinfo(long tid)        { return __sc(SYS_TINFO, tid, 0); }
static inline long u_irq_register(long irq)  { return __sc(SYS_IRQ_REGISTER, irq, 0); }
static inline long u_map_phys(long va, long pa){ return __sc(SYS_MAP_PHYS, va, pa); }
static inline long u_grant_cap(long tid, long ep){ return __sc(SYS_GRANT_CAP, tid, ep); }

static inline long          u_outb(long port, long val) { return __sc(SYS_OUTB, port, val); }
static inline unsigned char u_inb(long port)  { return (unsigned char)__sc(SYS_INB, port, 0); }
static inline unsigned short u_inw(long port) { return (unsigned short)__sc(SYS_INW, port, 0); }

static inline long u_grant_io(long tid, long base, long len)
{
    long r;
    __asm__ volatile ("int $0x80"
                      : "=a"(r)
                      : "a"((long)SYS_GRANT_IO), "D"(tid), "S"(base), "d"(len)
                      : "memory");
    return r;
}

static inline unsigned u_strlen(const char *s)
{
    unsigned n = 0;
    while (s[n]) n++;
    return n;
}

/* ---- printing via the user-space console driver (Phase 5) ---- */
static inline void con_write(const char *p, unsigned len)
{
    struct ipc_msg m = { .label = MSG_WRITE, .data0 = (unsigned long)p, .data1 = len };
    struct ipc_msg r;
    u_send(CON_TID, &m);
    u_recv(CON_TID, &r);
}
static inline void con_puts(const char *s) { con_write(s, u_strlen(s)); }

static inline unsigned con_readline(char *buf, unsigned max)
{
    struct ipc_msg m = { .label = MSG_READLINE, .data0 = (unsigned long)buf,
                         .data1 = max };
    struct ipc_msg r;
    u_send(CON_TID, &m);
    u_recv(CON_TID, &r);
    return (unsigned)r.data0;
}

static inline void con_putu(unsigned long v)
{
    char b[21]; int i = 0;
    if (!v) { con_write("0", 1); return; }
    while (v) { b[i++] = (char)('0' + v % 10); v /= 10; }
    char o[21]; int j = 0;
    while (i) o[j++] = b[--i];
    con_write(o, (unsigned)j);
}

static inline void con_puthex(unsigned long v)
{
    static const char d[] = "0123456789abcdef";
    char o[18]; o[0] = '0'; o[1] = 'x';
    for (int s = 60, k = 2; s >= 0; s -= 4) o[k++] = d[(v >> s) & 0xF];
    con_write(o, 18);
}

/* Compose a whole line in one buffer so concurrent printers never interleave
 * mid-line. con_linef appends a decimal, con_linex a hex value. */
static inline unsigned cl__cpy(char *b, unsigned k, const char *s)
{
    while (*s) b[k++] = *s++;
    return k;
}
static inline void con_linef(const char *a, unsigned long n, const char *b)
{
    char buf[160]; unsigned k = cl__cpy(buf, 0, a);
    char t[21]; int i = 0;
    if (!n) t[i++] = '0';
    while (n) { t[i++] = (char)('0' + n % 10); n /= 10; }
    while (i) buf[k++] = t[--i];
    k = cl__cpy(buf, k, b);
    con_write(buf, k);
}
static inline void con_linex(const char *a, unsigned long n, const char *b)
{
    static const char d[] = "0123456789abcdef";
    char buf[160]; unsigned k = cl__cpy(buf, 0, a);
    buf[k++] = '0'; buf[k++] = 'x';
    for (int s = 60; s >= 0; s -= 4) buf[k++] = d[(n >> s) & 0xF];
    k = cl__cpy(buf, k, b);
    con_write(buf, k);
}

static inline void u_puts(const char *s)    { while (*s) u_putc(*s++); }

static inline void u_putdec(unsigned long v)
{
    char b[21]; int i = 0;
    if (!v) { u_putc('0'); return; }
    while (v) { b[i++] = (char)('0' + v % 10); v /= 10; }
    while (i) u_putc(b[--i]);
}

static inline void u_puthex(unsigned long v)
{
    static const char d[] = "0123456789abcdef";
    u_puts("0x");
    for (int s = 60; s >= 0; s -= 4)
        u_putc(d[(v >> s) & 0xF]);
}
