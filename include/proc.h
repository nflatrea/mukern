/* include/proc.h - user-process bootstrap (kernel mechanism, Phase 4).
 *
 * The kernel's only "loader": it copies the flat server binaries from the
 * embedded rootfs into user pages and can create a Ring 3 thread for one.
 * All higher-level process policy lives in the user-space PM server.
 */
#pragma once

void rootfs_load_all(void);     /* map every rootfs program into user memory */
int  proc_spawn(int prog);      /* create a Ring 3 thread for PROG_* -> tid   */
void proc_start_init(void);     /* spawn PROG_INIT as the first user thread   */
