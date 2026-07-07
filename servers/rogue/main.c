/* servers/rogue/main.c - a deliberately buggy process (H2 fault isolation).
 *
 * It dereferences a wild pointer outside any valid region. The pager refuses
 * to map it and rules that the process be killed; the kernel tears it down and
 * the rest of the system keeps running. The "still alive" line must never
 * appear.
 */
#include "usys.h"

int main(void)
{
    con_puts("[rogue] touching a wild pointer (expecting to be killed)...\n");
    volatile unsigned long *bad = (volatile unsigned long *)0x0000700000000000ull;
    *bad = 1;
    con_puts("[rogue] STILL ALIVE - isolation FAILED\n");
    return 0;
}
