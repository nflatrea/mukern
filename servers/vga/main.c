/* servers/vga/main.c - VGA terminal: a second interactive console.
 *
 * Owns the VGA text buffer (output: a scrolling 80x25 text console) AND the
 * i8042 PS/2 keyboard (input: IRQ1, scancode set 1 -> ASCII). It speaks the
 * SAME protocol as the serial console driver - MSG_WRITE to print, MSG_READLINE
 * to read an edited line - so a shell bound to this server gets a full session
 * on the screen+keyboard, independent of the serial session. The kernel owns no
 * terminal policy; this Ring 3 server does.
 */
#include "usys.h"

/* ---- VGA text buffer (mapped at VGA_FB_VA by init) ---- */
#define COLS 80
#define ROWS 25
#define ATTR 0x07                               /* light grey on black */
static volatile unsigned short *const VGA = (volatile unsigned short *)VGA_FB_VA;
static int cur_r, cur_c;

static void scroll(void)
{
    for (int i = 0; i < (ROWS - 1) * COLS; i++) VGA[i] = VGA[i + COLS];
    for (int c = 0; c < COLS; c++) VGA[(ROWS - 1) * COLS + c] = (ATTR << 8) | ' ';
}
static void vga_putc(char ch)
{
    if (ch == '\n') { cur_c = 0; if (++cur_r >= ROWS) { scroll(); cur_r = ROWS - 1; } return; }
    if (ch == '\b') { if (cur_c > 0) { cur_c--; VGA[cur_r * COLS + cur_c] = (ATTR << 8) | ' '; } return; }
    if (ch == '\r') { cur_c = 0; return; }
    VGA[cur_r * COLS + cur_c] = (ATTR << 8) | (unsigned char)ch;
    if (++cur_c >= COLS) { cur_c = 0; if (++cur_r >= ROWS) { scroll(); cur_r = ROWS - 1; } }
}
static void vga_clear(void)
{
    for (int i = 0; i < COLS * ROWS; i++) VGA[i] = (ATTR << 8) | ' ';
    cur_r = cur_c = 0;
}

/* ---- i8042 PS/2 keyboard (scancode set 1) ---- */
#define KBD_DATA 0x60
static const char map_lo[0x3A] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};
static const char map_hi[0x3A] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};
static int shift;

/* decoded-key ring */
#define KRING 128
static char kr[KRING];
static unsigned kr_head, kr_tail;
static void kr_push(char c) { unsigned n = (kr_head + 1) % KRING; if (n != kr_tail) { kr[kr_head] = c; kr_head = n; } }
static int  kr_pop(void)    { if (kr_head == kr_tail) return -1; char c = kr[kr_tail]; kr_tail = (kr_tail + 1) % KRING; return c; }

static void kbd_drain(void)
{
    unsigned char sc = u_inb(KBD_DATA);
    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }       /* shift press   */
    if (sc == 0xAA || sc == 0xB6) { shift = 0; return; }       /* shift release */
    if (sc & 0x80) return;                                      /* other release */
    if (sc >= 0x3A) return;
    char c = shift ? map_hi[sc] : map_lo[sc];
    if (c) kr_push(c);
}

/* ---- pending line-read + simple editor ---- */
static int      rd_active;
static long     rd_from;
static char    *rd_buf;
static unsigned rd_max, rd_n;

static void edit(void)
{
    if (!rd_active) return;
    int c;
    while ((c = kr_pop()) >= 0) {
        if (c == '\n') {
            vga_putc('\n');
            rd_buf[rd_n] = 0;
            struct ipc_msg r = { .label = MSG_REPLY, .data0 = rd_n };
            long to = rd_from; rd_active = 0;
            u_send(to, &r);
            return;
        } else if (c == '\b') {
            if (rd_n) { rd_n--; vga_putc('\b'); }
        } else if (c >= 0x20 && c < 0x7f && rd_n + 1 < rd_max) {
            rd_buf[rd_n++] = (char)c;
            vga_putc((char)c);
        }
    }
}

int main(void)
{
    vga_clear();
    vga_putc('m'); vga_putc('u'); vga_putc('K'); vga_putc('e'); vga_putc('r'); vga_putc('n');
    for (const char *s = " VGA terminal -- type below\n"; *s; s++) vga_putc(*s);

    struct ipc_msg ready = { .label = MSG_READY };
    u_send(0, &ready);
    struct ipc_msg go; u_recv(0, &go);          /* wait for caps (kbd port, IRQ) */

    u_irq_register(1);                          /* keyboard IRQ */
    u_inb(KBD_DATA);                            /* drain any pending byte */

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_IRQ) {
            kbd_drain();
            edit();
        } else if (m.label == MSG_WRITE) {
            const char *p = (const char *)m.data0;
            unsigned len = (unsigned)m.data1;
            for (unsigned i = 0; i < len; i++) vga_putc(p[i]);
            struct ipc_msg r = { .label = MSG_REPLY };
            u_send(from, &r);
        } else if (m.label == MSG_READLINE) {
            rd_active = 1; rd_from = from;
            rd_buf = (char *)m.data0; rd_max = (unsigned)m.data1; rd_n = 0;
            edit();
        }
    }
    return 0;
}
