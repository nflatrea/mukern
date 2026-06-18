# muKern top-level build.
#
# Phase 1 implements x86_64 only (ROADMAP: "Do not attempt to support
# multiple architectures until Phase 6"). The arm64/riscv64 targets are
# placeholders that explain themselves rather than failing the build.
#
#   make            # build the x86_64 kernel
#   make run-x86_64 # boot it in QEMU (serial on stdio; quit with Ctrl-A x)
#   make clean

ARCH     ?= x86_64
BUILD    := build/$(ARCH)
KERNEL64 := $(BUILD)/mukern.elf64
KERNEL   := $(BUILD)/mukern.elf

.PHONY: all build clean run-x86_64 run-arm64 run-riscv64

ifeq ($(ARCH),x86_64)

CC      := gcc
LD      := ld
OBJCOPY := objcopy
QEMU    := qemu-system-x86_64

# Freestanding kernel: no libc/PIC/SSE, no red zone, no CET notes.
CFLAGS  := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pie -fno-pic -fcf-protection=none \
           -fno-asynchronous-unwind-tables \
           -mno-red-zone -mno-sse -mno-mmx -mno-80387 -mno-avx \
           -Wall -Wextra -O2 -g -I. -Iinclude -DARCH_x86_64
ASFLAGS := -DARCH_x86_64
LDFLAGS := -nostdlib -static -z noexecstack -T arch/x86_64/kernel.ld

SRCS := kernel/main.c \
        arch/x86_64/trap/idt.c \
        arch/x86_64/trap/trap.c \
        arch/x86_64/boot/header.S \
        arch/x86_64/boot/loader.S \
        arch/x86_64/trap/isr.S
OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SRCS)))

all: build
build: $(KERNEL)

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@

$(KERNEL64): $(OBJS) arch/x86_64/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# QEMU's Multiboot1 loader (and GRUB's) refuses a 64-bit ELF, so rewrite the
# ELF header to elf32-i386. The loadable bytes are untouched: entry is the
# 32-bit _start that switches the CPU into long mode.
$(KERNEL): $(KERNEL64)
	$(OBJCOPY) -O elf32-i386 $< $@
	@echo "built $@ (multiboot-bootable)"

# Serial multiplexed onto the terminal; -no-reboot so a fault is visible.
run-x86_64: build
	$(QEMU) -kernel $(KERNEL) -nographic -no-reboot

else  # ----- non-x86_64 ARCH -----

all build:
	@echo "ARCH=$(ARCH) is not implemented yet (ROADMAP Phase 6)."
	@echo "Phase 1 supports x86_64 only:  make ARCH=x86_64"

endif

run-arm64:
	@echo "arm64 port is ROADMAP Phase 6 - not yet implemented."

run-riscv64:
	@echo "riscv64 port is ROADMAP Phase 6 - not yet implemented."

clean:
	rm -rf build
