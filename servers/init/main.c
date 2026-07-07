/* servers/init/main.c - the first user process / system root.
 *
 * init is the only thread that boots as root. It brings up each server, and
 * before releasing it, endows it with exactly the capabilities it needs:
 *   - I/O-port caps for the drivers (H1),
 *   - endpoint (IPC send) caps for the services each server must call.
 * Every server announces readiness, then blocks until init grants its caps and
 * sends "go". A server can only message endpoints it was granted (plus replies
 * and init itself); the kernel enforces this. The kernel still draws nothing
 * and owns no policy.
 */
#include "usys.h"

#define HEAP_VA 0x0000400000400000ull

int main(void)
{
    struct ipc_msg m;
    struct ipc_msg go = { .label = MSG_REPLY };

    /* Console: owns the UART; needs no outgoing endpoint caps (replies only). */
    u_spawn(PROG_CONSOLE);
    u_recv(-1, &m);
    u_grant_io(CON_TID, 0x3F8, 8);
    u_send(CON_TID, &go);
    con_puts("[init] muKern booting; granting capabilities and starting services\n");

    /* Block driver: ATA ports + a console cap so it can log. */
    u_spawn(PROG_BLOCK);
    u_recv(-1, &m);
    u_grant_io(BLK_TID, 0x1F0, 8);
    u_grant_cap(BLK_TID, CON_TID);
    u_send(BLK_TID, &go);

    /* Pager: console cap. */
    u_spawn(PROG_VM);
    u_recv(-1, &m);
    u_grant_cap(VM_TID, CON_TID);
    u_send(VM_TID, &go);

    /* Process manager: console cap (also delegates it to children it spawns). */
    u_spawn(PROG_PM);
    u_recv(-1, &m);
    u_grant_cap(PM_TID, CON_TID);
    u_send(PM_TID, &go);

    /* Filesystem: needs the console and the block driver. */
    u_spawn(PROG_FS);
    u_recv(-1, &m);
    u_grant_cap(FS_TID, CON_TID);
    u_grant_cap(FS_TID, BLK_TID);
    u_send(FS_TID, &go);
    con_puts("[init] console, block, vm, pm, fs ready\n");

    /* VGA terminal: its own device memory (text buffer) + the PS/2 keyboard
     * I/O ports. It outputs to the screen and reads from the keyboard, so it
     * needs no serial-console cap. */
    u_map_phys(VGA_FB_VA, 0xB8000);
    u_spawn(PROG_VGA);
    u_recv(-1, &m);
    u_grant_io(VGA_TID, 0x60, 8);       /* i8042 data/status (0x60, 0x64) */
    u_send(VGA_TID, &go);
    con_puts("[init] serial console + vga terminal ready\n");

    /* Exercise the (capability-mediated) external pager once. */
    volatile unsigned long *p = (volatile unsigned long *)HEAP_VA;
    *p = 0xABCDul;
    con_linex("[init] demand-paged heap @ ", HEAP_VA,
              *p == 0xABCDul ? " OK\n" : " FAIL\n");

    /* Two independent shell sessions sharing fs + pm. Each is told which
     * terminal to use in its "go" message: one on the serial console, one on
     * the VGA terminal. */
    struct ipc_msg go_serial = { .label = MSG_REPLY, .data0 = CON_TID };
    long sh1 = u_spawn(PROG_SHELL);
    u_recv(-1, &m);
    u_grant_cap(sh1, CON_TID);
    u_grant_cap(sh1, FS_TID);
    u_grant_cap(sh1, PM_TID);
    u_send(sh1, &go_serial);

    struct ipc_msg go_vga = { .label = MSG_REPLY, .data0 = VGA_TID };
    long sh2 = u_spawn(PROG_SHELL2);
    u_recv(-1, &m);
    u_grant_cap(sh2, VGA_TID);
    u_grant_cap(sh2, FS_TID);
    u_grant_cap(sh2, PM_TID);
    u_send(sh2, &go_vga);
    con_puts("[init] two sessions up: shell on serial, shell on VGA+keyboard\n");

    u_recv(-1, &m);                     /* park */
    return 0;
}
