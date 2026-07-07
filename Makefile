# muKern top-level build.
#
# Per-architecture targets (no need to pass ARCH=... by hand):
#
#   make build-x86_64      make run-x86_64      make run-x86_64-vga
#   make build-riscv64     make run-riscv64
#   make build-all         make clean
#
# These dispatch to a per-ARCH sub-make. You can still drive it directly:
#   make ARCH=riscv64 build
#
# Porting model: there is one architecture-neutral kernel core (kernel/*.c) and
# a thin per-arch layer (arch/<arch>/) that provides boot, console, traps+timer,
# and the context-switch/stack hooks the scheduler needs. CORE_<arch> below lists
# how much of the neutral core each architecture currently builds; riscv64 is
# mid-port and grows toward the x86_64 set as paging, IPC and user mode land.

ARCH   ?= x86_64
BUILD  := build/$(ARCH)
KERNEL := $(BUILD)/mukern.elf

# Architecture-neutral kernel core, per arch (porting progress, made legible).
CORE_x86_64  := kernel/main.c kernel/vm.c kernel/sched.c kernel/ipc.c \
                kernel/cap.c kernel/proc.c
CORE_riscv64 := kernel/sched.c

# ===========================================================================
# Convenience targets: build-<arch>/run-<arch> dispatch to a per-ARCH sub-make.
# ===========================================================================
.PHONY: all build-all clean \
        build-x86_64 run-x86_64 run-x86_64-vga \
        build-riscv64 run-riscv64 \
        build-arm64 run-arm64 \
        build run run-vga

all: build-x86_64
build-all: build-x86_64 build-riscv64

build-x86_64:    ; @$(MAKE) ARCH=x86_64  build
run-x86_64:      ; @$(MAKE) ARCH=x86_64  run
run-x86_64-vga:  ; @$(MAKE) ARCH=x86_64  run-vga
build-riscv64:   ; @$(MAKE) ARCH=riscv64 build
run-riscv64:     ; @$(MAKE) ARCH=riscv64 run
build-arm64:     ; @$(MAKE) ARCH=arm64   build
run-arm64:       ; @$(MAKE) ARCH=arm64   run

clean:
	rm -rf build

# ===========================================================================
# Per-architecture configuration (selected by ARCH)
# ===========================================================================
ifeq ($(ARCH),x86_64)

CC      := gcc
LD      := ld
OBJCOPY := objcopy
QEMU    := qemu-system-x86_64
KERNEL64 := $(BUILD)/mukern.elf64

# User-program load addresses (shared by the user link and the kernel loader).
INIT_BASE  := 0x400000000000
PM_BASE    := 0x400000100000
HELLO_BASE := 0x400000200000
VM_BASE    := 0x400000300000
CON_BASE   := 0x400000500000
BLK_BASE   := 0x400000600000
FS_BASE    := 0x400000700000
APP_BASE   := 0x400000800000
ROGUE_BASE := 0x400000900000
SHELL_BASE := 0x400000a00000
VGA_BASE   := 0x400000c00000
SHELL2_BASE := 0x400000d00000

# Freestanding kernel: no libc/PIC/SSE, no red zone, no CET notes.
CFLAGS  := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pie -fno-pic -fcf-protection=none \
           -fno-asynchronous-unwind-tables \
           -mno-red-zone -mno-sse -mno-mmx -mno-80387 -mno-avx \
           -Wall -Wextra -O2 -g -I. -Iinclude -DARCH_x86_64 \
           -DINIT_BASE=$(INIT_BASE)ULL -DPM_BASE=$(PM_BASE)ULL \
           -DHELLO_BASE=$(HELLO_BASE)ULL -DVM_BASE=$(VM_BASE)ULL \
           -DCON_BASE=$(CON_BASE)ULL -DBLK_BASE=$(BLK_BASE)ULL \
           -DFS_BASE=$(FS_BASE)ULL -DAPP_BASE=$(APP_BASE)ULL -DROGUE_BASE=$(ROGUE_BASE)ULL -DSHELL_BASE=$(SHELL_BASE)ULL -DVGA_BASE=$(VGA_BASE)ULL -DSHELL2_BASE=$(SHELL2_BASE)ULL
ASFLAGS := -DARCH_x86_64
LDFLAGS := -nostdlib -static -z noexecstack -T arch/x86_64/kernel.ld

# Freestanding user programs (Ring 3), linked flat at fixed VAs.
UCFLAGS  := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector \
            -fno-pie -fno-pic -fcf-protection=none -mcmodel=large \
            -fno-asynchronous-unwind-tables \
            -mno-red-zone -mno-sse -mno-mmx -mno-80387 -mno-avx \
            -Wall -Wextra -O2 -Iinclude -Ilib
ULDFLAGS := -nostdlib -static -z noexecstack

ROOTFS    := $(BUILD)/rootfs
DISK      := $(BUILD)/disk.img
USER_BINS := $(ROOTFS)/init.bin $(ROOTFS)/pm.bin $(ROOTFS)/hello.bin \
             $(ROOTFS)/vm.bin $(ROOTFS)/console.bin $(ROOTFS)/block.bin \
             $(ROOTFS)/fs.bin $(ROOTFS)/app.bin $(ROOTFS)/rogue.bin \
             $(ROOTFS)/shell.bin $(ROOTFS)/vga.bin $(ROOTFS)/shell2.bin

SRCS := $(CORE_x86_64) \
        arch/x86_64/mm/memmap.c \
        arch/x86_64/mm/paging.c \
        arch/x86_64/trap/idt.c \
        arch/x86_64/trap/trap.c \
        arch/x86_64/trap/gdt.c \
        arch/x86_64/trap/usermode.c \
        arch/x86_64/trap/archstack.c \
        arch/x86_64/boot/header.S \
        arch/x86_64/boot/loader.S \
        arch/x86_64/trap/isr.S \
        arch/x86_64/trap/switch.S \
        kernel/rootfs.S
OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SRCS)))

build: $(KERNEL) $(DISK)

run: build
	$(QEMU) -kernel $(KERNEL) -drive file=$(DISK),format=raw,if=ide -nographic -no-reboot

run-vga: build
	$(QEMU) -kernel $(KERNEL) -drive file=$(DISK),format=raw,if=ide -vga std -serial stdio -no-reboot

ROOTDISK_SRCS := $(wildcard rootdisk/*)
$(DISK): $(ROOTDISK_SRCS)
	@mkdir -p $(dir $@)
	tar --format=ustar -C rootdisk -cf $@ $(notdir $(ROOTDISK_SRCS))
	@echo "built $@ (ustar disk image)"

# The embedded rootfs object incbin's the user binaries, so it depends on them.
$(BUILD)/kernel/rootfs.S.o: $(USER_BINS)

# ---- user-space build (crt0 + server main -> flat binary) ----
$(ROOTFS)/crt0.o: lib/crt0.S
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -DARCH_x86_64 -c $< -o $@
$(ROOTFS)/%.o: servers/%/main.c
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -c $< -o $@
$(ROOTFS)/vm.o: servers/vm_server/main.c
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -c $< -o $@
$(ROOTFS)/%.o: drivers/%/main.c
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -c $< -o $@
$(ROOTFS)/fs.o: servers/fs_server/main.c
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -c $< -o $@

$(ROOTFS)/shell2.o: servers/shell/main.c
	@mkdir -p $(dir $@)
	$(CC) $(UCFLAGS) -c $< -o $@

define USERBIN
$(ROOTFS)/$(1).bin: $(ROOTFS)/crt0.o $(ROOTFS)/$(1).o user/user.ld
	$(LD) $(ULDFLAGS) --defsym UBASE=$(2) -T user/user.ld -o $(ROOTFS)/$(1).elf $(ROOTFS)/crt0.o $(ROOTFS)/$(1).o
	$(OBJCOPY) -O binary $(ROOTFS)/$(1).elf $$@
endef
$(eval $(call USERBIN,init,$(INIT_BASE)))
$(eval $(call USERBIN,pm,$(PM_BASE)))
$(eval $(call USERBIN,hello,$(HELLO_BASE)))
$(eval $(call USERBIN,vm,$(VM_BASE)))
$(eval $(call USERBIN,console,$(CON_BASE)))
$(eval $(call USERBIN,block,$(BLK_BASE)))
$(eval $(call USERBIN,fs,$(FS_BASE)))
$(eval $(call USERBIN,app,$(APP_BASE)))
$(eval $(call USERBIN,rogue,$(ROGUE_BASE)))
$(eval $(call USERBIN,shell,$(SHELL_BASE)))
$(eval $(call USERBIN,vga,$(VGA_BASE)))
$(eval $(call USERBIN,shell2,$(SHELL2_BASE)))

$(KERNEL64): $(OBJS) arch/x86_64/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# QEMU's Multiboot1 loader refuses a 64-bit ELF, so rewrite the header to
# elf32-i386. Loadable bytes are untouched; entry is the 32-bit _start.
$(KERNEL): $(KERNEL64)
	$(OBJCOPY) -O elf32-i386 $< $@
	@echo "built $@ (multiboot-bootable)"

else ifeq ($(ARCH),riscv64)

CC   := riscv64-linux-gnu-gcc
LD   := riscv64-linux-gnu-ld
QEMU := qemu-system-riscv64

CFLAGS  := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pie -fno-pic -mcmodel=medany -march=rv64imac_zicsr_zifencei -mabi=lp64 \
           -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections \
           -Wall -Wextra -O2 -g -I. -Iinclude -DARCH_riscv64
ASFLAGS := -DARCH_riscv64 -march=rv64imac_zicsr_zifencei -mabi=lp64
LDFLAGS := -nostdlib -static --gc-sections -T arch/riscv64/kernel.ld

SRCS := $(CORE_riscv64) \
        arch/riscv64/kmain.c \
        arch/riscv64/trap/trap.c \
        arch/riscv64/trap/arch.c \
        arch/riscv64/boot/boot.S \
        arch/riscv64/trap/trap.S \
        arch/riscv64/trap/switch.S
OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SRCS)))

build: $(KERNEL)

run: build
	$(QEMU) -machine virt -bios default -kernel $(KERNEL) -nographic

$(KERNEL): $(OBJS) arch/riscv64/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "built $@ (riscv64 S-mode kernel)"

else  # ----- unimplemented ARCH (arm64: next Phase 6 milestone) -----

build run:
	@echo "ARCH=$(ARCH) is not implemented yet (arm64 is the next milestone)."
	@echo "Available:  make build-x86_64  |  make build-riscv64"

endif

# ===========================================================================
# Shared compile rules (use the per-ARCH CC/CFLAGS/ASFLAGS selected above)
# ===========================================================================
$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@
