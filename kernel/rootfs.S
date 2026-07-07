/* kernel/rootfs.S - embed the flat server binaries as the boot "rootfs".
 *
 * The .bin files are produced by the user-space build (see Makefile) and
 * incbin'd here. Paths are relative to the make working directory (repo root).
 */
.section .rodata
.balign 16

.global init_bin_start
.global init_bin_end
init_bin_start:  .incbin "build/x86_64/rootfs/init.bin"
init_bin_end:

.balign 16
.global pm_bin_start
.global pm_bin_end
pm_bin_start:    .incbin "build/x86_64/rootfs/pm.bin"
pm_bin_end:

.balign 16
.global hello_bin_start
.global hello_bin_end
hello_bin_start: .incbin "build/x86_64/rootfs/hello.bin"
hello_bin_end:

.balign 16
.global vm_bin_start
.global vm_bin_end
vm_bin_start:    .incbin "build/x86_64/rootfs/vm.bin"
vm_bin_end:

.balign 16
.global console_bin_start
.global console_bin_end
console_bin_start: .incbin "build/x86_64/rootfs/console.bin"
console_bin_end:

.balign 16
.global block_bin_start
.global block_bin_end
block_bin_start: .incbin "build/x86_64/rootfs/block.bin"
block_bin_end:

.balign 16
.global fs_bin_start
.global fs_bin_end
fs_bin_start:    .incbin "build/x86_64/rootfs/fs.bin"
fs_bin_end:

.balign 16
.global app_bin_start
.global app_bin_end
app_bin_start:   .incbin "build/x86_64/rootfs/app.bin"
app_bin_end:

.balign 16
.global rogue_bin_start
.global rogue_bin_end
rogue_bin_start: .incbin "build/x86_64/rootfs/rogue.bin"
rogue_bin_end:

.balign 16
.global shell_bin_start
.global shell_bin_end
shell_bin_start: .incbin "build/x86_64/rootfs/shell.bin"
shell_bin_end:

.balign 16
.global vga_bin_start
.global vga_bin_end
vga_bin_start:   .incbin "build/x86_64/rootfs/vga.bin"
vga_bin_end:

.balign 16
.global shell2_bin_start
.global shell2_bin_end
shell2_bin_start: .incbin "build/x86_64/rootfs/shell2.bin"
shell2_bin_end:
