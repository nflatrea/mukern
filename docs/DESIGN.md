# muKern design notes

This document explains the *why* behind the code. The headers in
`include/` explain the *what*.

## The three services, and nothing else

A microkernel is defined by what it refuses to contain. muKern's kernel
(`kernel/core/`) implements exactly three things:

1. **Tasks** — a thread of control with a stack and a saved context.
2. **A scheduler** — decides which task runs.
3. **IPC** — lets tasks exchange messages.

Device drivers, the console, and the shell are *not* in the kernel. They
are tasks (`kernel/srv/`, `kernel/core/repl.c`) that reach hardware only
by sending messages to driver servers. The only places the core touches a
device are the boot banner and `panic()`, which must work before the
scheduler exists and after everything else has failed — so they write the
UART directly through `kprintf`. That is a deliberate exception, not a
leak of policy into the kernel.

## IPC: synchronous rendezvous, no buffering

muKern follows the L4 family. There are four operations (`include/ipc.h`):

- `send`  — deliver a message, no reply expected.
- `recv`  — block until a message arrives (from anyone, or a named task).
- `call`  — atomic send-then-wait-for-reply (client RPC).
- `reply` — answer the task that `call`ed us.

Messages are fixed size: four machine words plus an optional `(ptr,len)`
for bulk data. There is **no kernel-side buffering** — a transfer happens
only when a sender and a receiver meet. This keeps the kernel allocation-
free and makes the cost of IPC obvious.

### The state machine

Each task owns one staging slot, `task->msg`. The relevant states are
`SENDBLK` (parked trying to send) and `RECVBLK` (parked waiting to
receive, *or* a caller waiting for a reply).

- **send to a waiting receiver** → fast path: copy the message into the
  receiver's slot, mark it runnable, done.
- **send with no receiver ready** → park the message in our own slot,
  enqueue ourselves on the destination's `senders` list, sleep.
- **recv** → if a matching sender is queued, take its message and wake it
  (or, for a `call`, move it to `RECVBLK` awaiting our reply); otherwise
  sleep until one arrives.
- **call** → exactly a `send` followed by transitioning ourselves to
  `RECVBLK` waiting for the callee's reply, so the whole RPC is atomic
  from the caller's side.
- **reply** → guarded by `dst->state == RECVBLK && dst->recv_from == me`,
  so a reply can only land on a task that is actually waiting for it.

A server is then just:

    for (;;) { ipc_recv(TID_ANY, &m); handle(&m); ipc_reply(m.from, &m); }

### Why it cannot deadlock

The runtime call graph is acyclic:

    shell -> console -> { serial, vga, kbd }

Each task only ever `call`s a task "below" it and `reply`s back up. The
shell performs one I/O operation at a time (it never holds a read open
while issuing a write), so there is no cycle of "A waits on B waits on A".
Combined with cooperative scheduling — a task runs until it blocks in IPC
or yields — control simply follows the message down the graph and the
reply follows it back.

## Scheduling

Cooperative round-robin. Task 0 is an idle task that never blocks, which
guarantees `pick_next()` always has something to return, so the scheduler
can never wedge with an empty run queue. Interrupts are effectively off
(traps are fatal), so on a single CPU no locking is needed anywhere —
another reason the code stays small. Preemption is milestone M8.

## Context switching

A task's entire machine context is the callee-saved registers spilled
onto its own stack; the saved stack pointer is the handle. `arch_switch`
pushes them, swaps `sp`, pops them, and `ret`s — resuming wherever the
target last switched out. `arch_ctx_init` forges a stack for a brand-new
task so the first switch "returns" into a trampoline that calls
`entry(arg)`. The three implementations live in `arch/<arch>/switch.S`
and `ctx.c`; the stack layouts were verified by hand for each ABI.

## The con protocol and the HAL

`con.h` is the one protocol every console-like device speaks: `PUTC`,
`PUTS`, `GETC`, `CLEAR`, and a non-blocking `POLL`. A driver implements
the subset it supports (the VGA server has no keyboard, the keyboard
server has no screen). The raw, per-arch register pokes live behind
`hal.h` (`uart_*`, `vga_*`, `kbd_*`) and are called *only* by the driver
servers and the emergency console — never by the portable core.

## Input multiplexing: waiting on two devices at once

Output is easy: the console server just forwards each write to the serial
driver and the VGA driver in turn. Input is the interesting case. A
blocking `GETC` can only wait on a single source, but the shell must
accept characters from *either* the serial line or the local keyboard,
whichever is typed first.

A blocking primitive cannot express "wait on A or B". So the console adds
a non-blocking op, `CON_POLL`: ask a device for a byte and get `-1`
immediately if none is ready. The console's `GETC` handler then sweeps
every input device and yields between sweeps:

    for (;;) {
        c = poll(serial); if (c >= 0) break;
        c = poll(kbd);    if (c >= 0) break;
        task_yield();
    }

This stays entirely within the cooperative, acyclic, deadlock-free model
— the console only ever calls drivers below it — yet it lets one shell be
driven from a serial terminal and a PS/2 keyboard simultaneously. Adding
a third input source later is one more `poll()` line. (When preemption and
interrupt forwarding arrive in a later milestone, the same `con.h`
protocol can sit on top of IRQ-driven drivers that block instead of poll;
only the driver internals change.)

## A note on the VGA driver

The x86_64 VGA text buffer is at the identity-mapped `0xB8000`. The driver
keeps a small RAM shadow of the screen and treats it as the source of
truth, blitting changes through to the live framebuffer. Scrolling shifts
the shadow and re-blits. This is deliberate: under the standard VGA's
odd/even text addressing, a CPU *read* of the framebuffer does not
reliably return the character/attribute pair that was written, so a
read-modify scroll would corrupt the screen. Writing-only, with a shadow,
sidesteps the whole problem.
