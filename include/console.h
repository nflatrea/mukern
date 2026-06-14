/*
file: console.h
desc: portable arch interface for the uKern serial PoC.
      Every architecture under arch/<arch>/ implements exactly these six
      functions. The portable core (kernel/core/) depends on nothing else.
      This is the entire HAL: keep it that way.
*/
#ifndef MUKERN_CONSOLE_H
#define MUKERN_CONSOLE_H

void        console_init(void);   /* bring the UART up (may be a no-op)      */
void        console_putc(char c); /* blocking write of one byte              */
int         console_getc(void);   /* non-blocking read; returns -1 if empty  */
const char *arch_name(void);      /* human-readable arch string              */
void        arch_halt(void) __attribute__((noreturn)); /* park CPU forever  */

#endif /* MUKERN_CONSOLE_H */
