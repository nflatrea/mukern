/*
file: arch/x86_64/kbd.c
desc: PS/2 keyboard back-end (hal.h), x86_64 only. Polls the i8042 controller
      (data 0x60, status 0x64) and translates scan-code set 1 into ASCII,
      tracking shift and caps-lock. Like the other HAL back-ends this is raw
      hardware glue owned at runtime by a driver SERVER task (kernel/srv/kbd.c)
      and reached only through con.h; the kernel core never calls it.

      Input is polled, not interrupt-driven, to match the rest of muKern
      while traps are still fatal (see ROADMAP: interrupt forwarding is a
      later milestone). kbd_getc() never blocks -- it returns -1 when no key
      is ready -- so the console server can interleave it with the serial
      line and wait on both at once.
*/
#include <stdint.h>
#include "hal.h"

#define KBD_DATA 0x60
#define KBD_STAT 0x64

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

/* scan-code set 1, make codes 0x00..0x39 -> ASCII (US layout). 0 = ignore. */
static const char base[0x3A] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',
    [0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',[0x0C]='-',[0x0D]='=',
    [0x0E]='\b',[0x0F]='\t',
    [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',
    [0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',[0x1A]='[',[0x1B]=']',
    [0x1C]='\n',
    [0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',
    [0x24]='j',[0x25]='k',[0x26]='l',[0x27]=';',[0x28]='\'',[0x29]='`',
    [0x2B]='\\',
    [0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',
    [0x32]='m',[0x33]=',',[0x34]='.',[0x35]='/',
    [0x39]=' ',
};

static const char shifted[0x3A] = {
    [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',[0x07]='^',
    [0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',[0x0C]='_',[0x0D]='+',
    [0x0E]='\b',[0x0F]='\t',
    [0x10]='Q',[0x11]='W',[0x12]='E',[0x13]='R',[0x14]='T',[0x15]='Y',
    [0x16]='U',[0x17]='I',[0x18]='O',[0x19]='P',[0x1A]='{',[0x1B]='}',
    [0x1C]='\n',
    [0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',
    [0x24]='J',[0x25]='K',[0x26]='L',[0x27]=':',[0x28]='"',[0x29]='~',
    [0x2B]='|',
    [0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',[0x30]='B',[0x31]='N',
    [0x32]='M',[0x33]='<',[0x34]='>',[0x35]='?',
    [0x39]=' ',
};

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_CAPS   0x3A
#define SC_RELEASE 0x80           /* high bit set on a break (key-up) code */

static int shift, caps;

void kbd_init(void)
{
    /* Drain anything the firmware left in the output buffer. */
    while (inb(KBD_STAT) & 0x01)
        (void)inb(KBD_DATA);
    shift = caps = 0;
}

int kbd_getc(void)
{
    uint8_t st = inb(KBD_STAT);
    if (!(st & 0x01))             /* output buffer empty: nothing to read   */
        return -1;
    uint8_t sc = inb(KBD_DATA);
    if (st & 0x20)                /* AUX byte (mouse): not ours, ignore      */
        return -1;

    if (sc & SC_RELEASE) {        /* key-up: only shift state matters        */
        uint8_t code = sc & 0x7F;
        if (code == SC_LSHIFT || code == SC_RSHIFT)
            shift = 0;
        return -1;
    }
    if (sc == SC_LSHIFT || sc == SC_RSHIFT) { shift = 1; return -1; }
    if (sc == SC_CAPS) { caps = !caps; return -1; }
    if (sc >= 0x3A)               /* function/keypad/etc: unmapped           */
        return -1;

    char lo = base[sc];
    if (lo >= 'a' && lo <= 'z')   /* letters: caps and shift combine (xor)   */
        return (shift ^ caps) ? shifted[sc] : lo;
    return shift ? shifted[sc] : lo;   /* others: shift only                 */
}
