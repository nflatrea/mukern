/* servers/fs_server/main.c - filesystem server: USTAR over IPC (M5c).
 *
 * A user-space VFS. It reads the on-disk USTAR archive through the block
 * driver (itself a user process) and serves open()/read() requests over IPC.
 * The kernel knows nothing about files or disks.
 */
#include "usys.h"

static int blk_read(unsigned long lba, void *buf)
{
    struct ipc_msg m = { .label = MSG_BREAD, .data0 = lba,
                         .data1 = (unsigned long)buf };
    struct ipc_msg r;
    u_send(BLK_TID, &m);
    u_recv(BLK_TID, &r);
    return (int)(long)r.data0;
}

static unsigned long parse_octal(const char *p, int n)
{
    unsigned long v = 0;
    for (int i = 0; i < n && p[i] >= '0' && p[i] <= '7'; i++)
        v = (v << 3) + (unsigned long)(p[i] - '0');
    return v;
}

static int name_eq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

struct fent { unsigned long data_lba, size; int used; };
static struct fent ftab[8];
static unsigned char hdr[512];
static unsigned char bounce[512];

/* Walk USTAR headers to find `path`; record a handle. */
static long fs_open(const char *path)
{
    unsigned long lba = 0;
    for (;;) {
        if (blk_read(lba, hdr) != 0 || hdr[0] == 0)
            return -1;                      /* error or end of archive */
        unsigned long size = parse_octal((char *)hdr + 124, 12);
        unsigned long data_lba = lba + 1;
        if (name_eq((char *)hdr, path)) {
            for (int i = 0; i < 8; i++) if (!ftab[i].used) {
                ftab[i].data_lba = data_lba;
                ftab[i].size = size;
                ftab[i].used = 1;
                return i;
            }
            return -1;                      /* handle table full */
        }
        lba = data_lba + (size + 511) / 512; /* skip this file's data */
    }
}

static long fs_read(long h, unsigned char *buf, unsigned long max)
{
    if (h < 0 || h >= 8 || !ftab[h].used)
        return -1;
    unsigned long size = ftab[h].size;
    if (size > max) size = max;             /* never overrun the caller buffer */
    unsigned long full = size / 512, rem = size - full * 512;
    for (unsigned long i = 0; i < full; i++)
        blk_read(ftab[h].data_lba + i, buf + i * 512);
    if (rem) {                              /* partial last sector via bounce */
        blk_read(ftab[h].data_lba + full, bounce);
        for (unsigned long j = 0; j < rem; j++) buf[full * 512 + j] = bounce[j];
    }
    return (long)size;
}

/* List archive entries as newline-separated names into buf; returns length. */
static long fs_list(char *buf, unsigned long max)
{
    unsigned long lba = 0, out = 0;
    for (;;) {
        if (blk_read(lba, hdr) != 0 || hdr[0] == 0)
            break;
        const char *name = (const char *)hdr;
        for (int i = 0; name[i] && out + 2 < max; i++)
            buf[out++] = name[i];
        if (out + 1 < max) buf[out++] = '\n';
        unsigned long size = parse_octal((char *)hdr + 124, 12);
        lba += 1 + (size + 511) / 512;
    }
    buf[out] = 0;
    return (long)out;
}

int main(void)
{
    struct ipc_msg ready = { .label = MSG_READY };
    u_send(0, &ready);
    struct ipc_msg go; u_recv(0, &go);
    con_linef("[fs] filesystem server up (tid=", (unsigned long)u_gettid(), ")\n");

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_OPEN) {
            long h = fs_open((const char *)m.data0);
            struct ipc_msg r = { .label = MSG_REPLY,
                                 .data0 = (unsigned long)h,
                                 .data1 = (h >= 0) ? ftab[h].size : 0 };
            u_send(from, &r);
        } else if (m.label == MSG_FREAD) {
            unsigned long max = m.cap ? m.cap : ftab[0].size;
            long n = fs_read((long)m.data0, (unsigned char *)m.data1, max);
            struct ipc_msg r = { .label = MSG_REPLY, .data0 = (unsigned long)n };
            u_send(from, &r);
        } else if (m.label == MSG_CLOSE) {
            long h = (long)m.data0;
            if (h >= 0 && h < 8) ftab[h].used = 0;
            struct ipc_msg r = { .label = MSG_REPLY };
            u_send(from, &r);
        } else if (m.label == MSG_LIST) {
            long n = fs_list((char *)m.data0, m.data1);
            struct ipc_msg r = { .label = MSG_REPLY, .data0 = (unsigned long)n };
            u_send(from, &r);
        }
    }
    return 0;
}
