/* include/ipc.h - synchronous (rendezvous) IPC ABI + kernel API (M3a).
 *
 * Per DESIGN: no kernel buffering. A small message rides in a few machine
 * words (label + two data words, plus a capability slot used from M3b). The
 * sender blocks until the receiver is ready and vice-versa.
 */
#pragma once
#include "types.h"

#define IPC_ANY (-1)            /* receive from any sender */

/* Register-sized message (DESIGN: label=arg0, data=arg1/arg2, cap=arg3). */
struct ipc_msg {
    uint64_t label;
    uint64_t data0;
    uint64_t data1;
    uint64_t cap;
};

/* IPC blocking state stored on a thread. */
enum {
    IPC_NONE = 0,
    IPC_SENDING,
    IPC_RECEIVING,
};

/* Kernel-thread IPC primitives. Return 0 on success, negative on error. */
int ipc_send(int dest_tid, const struct ipc_msg *msg);
int ipc_recv(int src_tid, struct ipc_msg *out);

/* Lock-free variants for callers already running with interrupts disabled
 * (the syscall and page-fault paths). ipc_recv_locked returns the sender's
 * tid (>= 0) on success. `self` is the calling thread. */
struct thread;
int ipc_send_locked(struct thread *self, int dest_tid, const struct ipc_msg *msg);
int ipc_recv_locked(struct thread *self, int src_tid, struct ipc_msg *out);

/* ---- Message labels used by the Phase 4 servers (user/user protocol) ---- */
#define MSG_READY  1            /* server -> init: I am up                  */
#define MSG_SPAWN  2            /* init -> pm: spawn program data0           */
#define MSG_REPLY  3            /* pm -> init: result in data0              */
#define MSG_FAULT  4            /* kernel -> pager: data0=tid, data1=addr   */

/* ---- Phase 5 driver/server protocol labels ---- */
#define MSG_WRITE  10           /* -> console: data0=buf ptr, data1=len     */
#define MSG_BREAD  11           /* -> block: data0=lba, data1=buf ptr (512)  */
#define MSG_OPEN   12           /* -> fs: data0=path ptr; reply h+size       */
#define MSG_FREAD  13           /* -> fs: data0=handle, data1=buf, cap=max    */
#define MSG_READLINE 14         /* -> console: data0=buf, data1=max; reply n */
#define MSG_LIST   15           /* -> fs: data0=buf, data1=max; reply nbytes  */
#define MSG_CLOSE  16           /* -> fs: data0=handle; release it            */
#define MSG_IRQ    20           /* kernel -> driver: a registered IRQ fired   */

/* Sentinel "sender" id returned by ipc_recv for a kernel IRQ notification. */
#define IPC_IRQ_SENDER (-2)
#define MSG_VPUTS  21           /* -> vga: data0=str ptr, data1=row           */
