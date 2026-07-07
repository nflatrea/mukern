/* drivers/block/main.c - user-space ATA (IDE) block driver, PIO + polling (M5b).
 *
 * Reads 512-byte sectors from the primary master via the port-I/O syscalls and
 * serves them over IPC (MSG_BREAD: lba + destination buffer in the shared
 * address space). No interrupts: the driver polls the status register.
 */
#include "usys.h"

#define ATA_DATA   0x1F0
#define ATA_SECCNT 0x1F2
#define ATA_LBA0   0x1F3
#define ATA_LBA1   0x1F4
#define ATA_LBA2   0x1F5
#define ATA_DRIVE  0x1F6
#define ATA_CMD    0x1F7        /* write: command   */
#define ATA_STATUS 0x1F7        /* read:  status    */
#define ST_ERR  0x01
#define ST_DRQ  0x08
#define ST_BSY  0x80

static int ata_read1(unsigned long lba, unsigned short *buf)
{
    while (u_inb(ATA_STATUS) & ST_BSY) { }
    u_outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));     /* master, LBA28 */
    u_outb(ATA_SECCNT, 1);
    u_outb(ATA_LBA0, lba & 0xFF);
    u_outb(ATA_LBA1, (lba >> 8) & 0xFF);
    u_outb(ATA_LBA2, (lba >> 16) & 0xFF);
    u_outb(ATA_CMD, 0x20);                              /* READ SECTORS */

    unsigned char s;
    do { s = u_inb(ATA_STATUS); }
    while ((s & ST_BSY) || (!(s & ST_DRQ) && !(s & ST_ERR)));
    if (s & ST_ERR)
        return -1;

    for (int i = 0; i < 256; i++)
        buf[i] = u_inw(ATA_DATA);
    return 0;
}

int main(void)
{
    /* Announce, then wait for init to grant the ATA ports before any I/O. */
    struct ipc_msg ready = { .label = MSG_READY };
    u_send(0, &ready);
    struct ipc_msg grant;
    u_recv(0, &grant);

    con_linef("[blk] ATA block driver up (tid=", (unsigned long)u_gettid(), ")\n");

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_BREAD) {
            int rc = ata_read1(m.data0, (unsigned short *)m.data1);
            struct ipc_msg r = { .label = MSG_REPLY, .data0 = (unsigned long)(long)rc };
            u_send(from, &r);
        }
    }
    return 0;
}
