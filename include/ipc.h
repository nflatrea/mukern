/*
file: ipc.h
desc: Synchronous message-passing IPC -- the defining service of the kernel.

      muKern follows the L4 family: a tiny set of blocking rendezvous
      primitives, fixed-size messages, no buffering in the kernel. A message
      is a handful of machine words (the "registers") plus an optional
      (ptr,len) for bulk data. Today every task shares one address space, so
      ptr is passed verbatim; under per-task address spaces (a later
      milestone) the kernel would copy the buffer across the boundary -- the
      protocol does not change.

      Four operations cover every interaction:

        send  -> client notification, no reply expected
        recv  -> a server waits for the next request
        call  -> client RPC: atomic send-then-wait-for-reply
        reply -> server answers the caller of an earlier call

      call/reply are the workhorses: the shell calls the console server,
      which calls the serial and vga drivers. Plain send is there for
      fire-and-forget notifications.
*/
#ifndef MUKERN_IPC_H
#define MUKERN_IPC_H

#include <stdint.h>
#include "task.h"   /* tid_t, TID_ANY, TID_NONE */

#define MSG_WORDS 4

typedef struct {
    tid_t    from;            /* sender's tid; filled in by the kernel       */
    uint32_t tag;             /* operation label, defined by the protocol    */
    uint64_t w[MSG_WORDS];    /* inline data words ("message registers")     */
    void    *ptr;             /* optional bulk pointer (valid: shared AS)    */
    uint64_t len;             /* length that goes with ptr                   */
} msg_t;

/* Block until dst receives *m. No reply is awaited. */
int ipc_send(tid_t dst, const msg_t *m);

/* Block until a message arrives. from==TID_ANY accepts any sender; a
 * specific tid accepts only that sender. The message (incl. .from) lands
 * in *m. */
int ipc_recv(tid_t from, msg_t *m);

/* Client RPC: atomically send *m to dst and block for dst's reply, which
 * overwrites *m. */
int ipc_call(tid_t dst, msg_t *m);

/* Server side: answer the task that called us. Fails (-1) if dst is not
 * actually waiting for our reply. */
int ipc_reply(tid_t dst, const msg_t *m);

#endif /* MUKERN_IPC_H */
