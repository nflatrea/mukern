/* drivers/console/main.c - user-space console driver (interrupt-driven I/O).
 *
 * Owns the 16550 UART. Output is synchronous (MSG_WRITE). Input is
 * interrupt-driven: arriving bytes are drained into a ring buffer on IRQ4 and
 * a MSG_READLINE request is served from that buffer (type-ahead included).
 *
 * The driver is a full single-line editor: it parses ANSI escape sequences so
 * arrow keys never leak to the terminal, supports in-line editing (left/right,
 * home/end, backspace, delete) and a command history (up/down). All terminal
 * handling lives here, where the hardware is owned; clients just get a line.
 */
#include "usys.h"

#define UART_DATA 0x3F8
#define UART_IER  0x3F9
#define UART_LSR  0x3FD
#define LSR_RX    0x01
#define LSR_THRE  0x20

static void uart_putc(char c)
{
    while (!(u_inb(UART_LSR) & LSR_THRE)) { }
    u_outb(UART_DATA, (unsigned char)c);
}
static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
static void uart_putu(unsigned long v)
{
    char b[21]; int i = 0;
    if (!v) { uart_putc('0'); return; }
    while (v) { b[i++] = (char)('0' + v % 10); v /= 10; }
    while (i) uart_putc(b[--i]);
}
static void backspaces(unsigned k) { while (k--) uart_putc('\b'); }

/* ---- RX ring buffer (filled from IRQ, drained by the editor) ---- */
#define RING 256
static unsigned char ring[RING];
static unsigned r_head, r_tail;
static void ring_push(unsigned char c)
{
    unsigned n = (r_head + 1) % RING;
    if (n != r_tail) { ring[r_head] = c; r_head = n; }
}
static int ring_pop(void)
{
    if (r_head == r_tail) return -1;
    unsigned char c = ring[r_tail];
    r_tail = (r_tail + 1) % RING;
    return c;
}
static void drain_rx(void)
{
    while (u_inb(UART_LSR) & LSR_RX) ring_push(u_inb(UART_DATA));
}

/* ---- command history (newest-first ring) ---- */
#define HMAX 16
#define HLEN 128
static char     hist[HMAX][HLEN];
static unsigned hist_len[HMAX];
static int      h_count;             /* stored entries (<= HMAX)        */
static int      h_next;              /* where the next entry goes       */
static int      h_nav;              /* 0 = live line, else entry depth */
static char     h_saved[HLEN];       /* the in-progress line, parked     */
static unsigned h_saved_n;

static void hist_push(const char *s, unsigned n)
{
    if (n == 0) return;
    int newest = (h_next - 1 + HMAX) % HMAX;     /* skip consecutive dups */
    if (h_count && hist_len[newest] == n) {
        unsigned i = 0; while (i < n && hist[newest][i] == s[i]) i++;
        if (i == n) return;
    }
    if (n >= HLEN) n = HLEN - 1;
    for (unsigned i = 0; i < n; i++) hist[h_next][i] = s[i];
    hist_len[h_next] = n;
    h_next = (h_next + 1) % HMAX;
    if (h_count < HMAX) h_count++;
}

/* ---- pending line-read request + editor state ---- */
static int      rd_active;
static long     rd_from;
static char    *rd_buf;
static unsigned rd_max, rd_n, rd_pos;
static int      esc;                 /* 0 normal, 1 saw ESC, 2 in CSI   */
static int      csi_num;

/* Rewrite buf[from..n], then leave the cursor at rd_pos. */
static void redraw_from(unsigned from)
{
    for (unsigned i = from; i < rd_n; i++) uart_putc(rd_buf[i]);
    backspaces(rd_n - rd_pos);
}
static void ins_char(char c)
{
    if (rd_n + 1 >= rd_max) return;
    for (unsigned i = rd_n; i > rd_pos; i--) rd_buf[i] = rd_buf[i - 1];
    rd_buf[rd_pos] = c; rd_n++;
    for (unsigned i = rd_pos; i < rd_n; i++) uart_putc(rd_buf[i]);
    rd_pos++;
    backspaces(rd_n - rd_pos);
}
static void del_left(void)           /* backspace */
{
    if (rd_pos == 0) return;
    for (unsigned i = rd_pos - 1; i < rd_n - 1; i++) rd_buf[i] = rd_buf[i + 1];
    rd_n--; rd_pos--;
    uart_putc('\b');
    redraw_from(rd_pos); uart_putc(' '); backspaces(rd_n - rd_pos + 1);
}
static void del_at(void)             /* delete key */
{
    if (rd_pos >= rd_n) return;
    for (unsigned i = rd_pos; i < rd_n - 1; i++) rd_buf[i] = rd_buf[i + 1];
    rd_n--;
    redraw_from(rd_pos); uart_putc(' '); backspaces(rd_n - rd_pos + 1);
}
static void go_left(void)  { if (rd_pos) { rd_pos--; uart_putc('\b'); } }
static void go_right(void) { if (rd_pos < rd_n) uart_putc(rd_buf[rd_pos++]); }
static void go_home(void)  { while (rd_pos) { rd_pos--; uart_putc('\b'); } }
static void go_end(void)   { while (rd_pos < rd_n) uart_putc(rd_buf[rd_pos++]); }

/* Erase the visible line and load a new one. */
static void set_line(const char *s, unsigned n)
{
    go_end();
    while (rd_n) { uart_puts("\b \b"); rd_n--; }
    if (n >= rd_max) n = rd_max - 1;
    for (unsigned i = 0; i < n; i++) rd_buf[i] = s[i];
    rd_n = rd_pos = n;
    for (unsigned i = 0; i < rd_n; i++) uart_putc(rd_buf[i]);
}
static void hist_nav(int delta)      /* -1 = older (up), +1 = newer (down) */
{
    int nav = h_nav + (delta < 0 ? 1 : -1);
    if (nav < 0 || nav > h_count) return;
    if (h_nav == 0 && nav > 0) {                 /* park the live line */
        h_saved_n = rd_n < HLEN ? rd_n : HLEN - 1;
        for (unsigned i = 0; i < h_saved_n; i++) h_saved[i] = rd_buf[i];
    }
    h_nav = nav;
    if (nav == 0) { set_line(h_saved, h_saved_n); return; }
    int idx = (h_next - nav + HMAX) % HMAX;
    set_line(hist[idx], hist_len[idx]);
}

static void finish_line(void)
{
    uart_putc('\n');
    rd_buf[rd_n] = 0;
    hist_push(rd_buf, rd_n);
    struct ipc_msg r = { .label = MSG_REPLY, .data0 = rd_n };
    long to = rd_from;
    rd_active = 0;
    u_send(to, &r);
}

static void edit(void)
{
    if (!rd_active) return;
    int c;
    while ((c = ring_pop()) >= 0) {
        if (esc == 1) {                          /* after ESC */
            esc = (c == '[' || c == 'O') ? 2 : 0;
            csi_num = 0;
            continue;
        }
        if (esc == 2) {                          /* inside CSI */
            if (c >= '0' && c <= '9') { csi_num = csi_num * 10 + (c - '0'); continue; }
            switch (c) {
            case 'A': hist_nav(-1); break;
            case 'B': hist_nav(+1); break;
            case 'C': go_right();   break;
            case 'D': go_left();    break;
            case 'H': go_home();    break;
            case 'F': go_end();     break;
            case '~':
                if      (csi_num == 3) del_at();
                else if (csi_num == 1 || csi_num == 7) go_home();
                else if (csi_num == 4 || csi_num == 8) go_end();
                break;
            }
            esc = 0;
            continue;
        }
        switch (c) {                             /* normal */
        case 0x1b: esc = 1; break;
        case '\r': case '\n': finish_line(); return;
        case 0x7f: case '\b': del_left(); break;
        case 0x01: go_home(); break;             /* Ctrl-A */
        case 0x05: go_end();  break;             /* Ctrl-E */
        case 0x02: go_left(); break;             /* Ctrl-B */
        case 0x06: go_right();break;             /* Ctrl-F */
        case 0x15:                               /* Ctrl-U: kill line */
            go_end(); while (rd_n) { uart_puts("\b \b"); rd_n--; } rd_pos = 0; break;
        case 0x03:                               /* Ctrl-C: cancel */
            uart_puts("^C\n"); rd_n = rd_pos = 0; finish_line(); return;
        default:
            if (c >= 0x20 && c < 0x7f) ins_char((char)c);
        }
    }
}

int main(void)
{
    struct ipc_msg ready = { .label = MSG_READY };
    u_send(0, &ready);
    struct ipc_msg grant;
    u_recv(0, &grant);

    u_irq_register(4);
    u_outb(UART_IER, 0x01);
    drain_rx();

    uart_puts("[con] console driver up (tid=");
    uart_putu((unsigned long)u_gettid());
    uart_puts(", line editor + history)\n");

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_IRQ) {
            drain_rx();
            edit();
        } else if (m.label == MSG_WRITE) {
            const char *p = (const char *)m.data0;
            unsigned len = (unsigned)m.data1;
            for (unsigned i = 0; i < len; i++) uart_putc(p[i]);
            struct ipc_msg r = { .label = MSG_REPLY };
            u_send(from, &r);
        } else if (m.label == MSG_READLINE) {
            rd_active = 1; rd_from = from;
            rd_buf = (char *)m.data0; rd_max = (unsigned)m.data1;
            rd_n = rd_pos = 0; esc = 0; h_nav = 0;
            edit();                              /* serve from type-ahead */
        }
    }
    return 0;
}
