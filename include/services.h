/*
file: services.h
desc: The initial task's tiny service registry.

      In a microkernel the very first task is the one that knows where the
      servers live and hands those endpoints to everyone else. Here, with a
      single address space, that "name service" collapses to three globals
      published by kernel/core/init.c. A task reads the endpoint it needs and
      then speaks the con.h protocol to it.

      svc_vga is TID_NONE on arches without a framebuffer; the console
      multiplexer simply skips it.
*/
#ifndef MUKERN_SERVICES_H
#define MUKERN_SERVICES_H

#include "task.h"

extern tid_t svc_serial;    /* serial (UART) driver server   */
extern tid_t svc_vga;       /* vga text driver server, or TID_NONE */
extern tid_t svc_kbd;       /* ps/2 keyboard driver server, or TID_NONE */
extern tid_t svc_console;   /* console multiplexer (the shell's endpoint) */

#endif /* MUKERN_SERVICES_H */
