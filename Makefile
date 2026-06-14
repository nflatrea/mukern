# file: Makefile
#
#   make ARCH=x86_64        build one arch (x86_64 | arm64 | riscv64)
#   make all                build all three
#   make run-x86_64         build + boot in QEMU (-nographic)
#   make run-arm64
#   make run-riscv64
#   make clean
#
# Cross toolchains expected on PATH (see INSTALL):
#   x86_64 	: host gcc (freestanding)
#   arm64   : aarch64-linux-gnu-gcc
#   riscv64 : riscv64-linux-gnu-gcc

ARCH    ?= x86_64
BUILD   := build/$(ARCH)
CORE    := kernel/core/main.c kernel/core/repl.c kernel/core/panic.c
LIBK    := kernel/lib/string.c kernel/lib/kprintf.c
INC     := -Iinclude

CFLAGS_COMMON := -ffreestanding -nostdlib -fno-stack-protector \
                 -fno-pic -fno-pie -fno-builtin -Wall -Wextra -O2 -g $(INC)
LDCOMMON      := -Wl,--build-id=none -Wl,-z,noexecstack

ifeq ($(ARCH),x86_64)
  CC      := gcc
  CFLAGS  := $(CFLAGS_COMMON) -m64 -mno-red-zone -mno-sse -mno-mmx -mno-80387
  LDFLAGS := -no-pie -Wl,-n
  QEMU    := qemu-system-x86_64
  QFLAGS  := -kernel $(BUILD)/kernel.elf -nographic -no-reboot
else ifeq ($(ARCH),arm64)
  CC      := aarch64-linux-gnu-gcc
  CFLAGS  := $(CFLAGS_COMMON) -mgeneral-regs-only
  LDFLAGS := -static
  QEMU    := qemu-system-aarch64
  QFLAGS  := -M virt -cpu cortex-a72 -nographic -kernel $(BUILD)/kernel.elf
else ifeq ($(ARCH),riscv64)
  CC      := riscv64-linux-gnu-gcc
  CFLAGS  := $(CFLAGS_COMMON) -mcmodel=medany
  LDFLAGS := -static
  QEMU    := qemu-system-riscv64
  QFLAGS  := -M virt -bios default -nographic -kernel $(BUILD)/kernel.elf
else
  $(error unknown ARCH '$(ARCH)'; use x86_64 | arm64 | riscv64)
endif

SRCS := arch/$(ARCH)/boot.S arch/$(ARCH)/serial.c arch/$(ARCH)/trap.S \
        arch/$(ARCH)/trap.c $(CORE) $(LIBK)
LD   := arch/$(ARCH)/linker.ld

# x86_64: QEMU's multiboot loader needs a 32-bit ELF container, so link
# ELF64 then re-wrap as elf32-i386 (machine code is unchanged).
ifeq ($(ARCH),x86_64)
  LINKOUT := $(BUILD)/kernel.elf64
  POST     = objcopy -O elf32-i386 $(BUILD)/kernel.elf64 $(BUILD)/kernel.elf
else
  LINKOUT := $(BUILD)/kernel.elf
  POST     = true
endif

.PHONY: all build run-x86_64 run-arm64 run-riscv64 clean

build: $(BUILD)/kernel.elf

$(BUILD)/kernel.elf: $(SRCS) $(LD)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDCOMMON) -T $(LD) -o $(LINKOUT) $(SRCS)
	$(POST)
	@echo "built $@"

all:
	$(MAKE) ARCH=x86_64  build
	$(MAKE) ARCH=arm64   build
	$(MAKE) ARCH=riscv64 build

run-x86_64:  ; $(MAKE) ARCH=x86_64  build && qemu-system-x86_64  -kernel build/x86_64/kernel.elf -nographic -no-reboot
run-arm64:   ; $(MAKE) ARCH=arm64   build && qemu-system-aarch64 -M virt -cpu cortex-a72 -nographic -kernel build/arm64/kernel.elf
run-riscv64: ; $(MAKE) ARCH=riscv64 build && qemu-system-riscv64 -M virt -bios default -nographic -kernel build/riscv64/kernel.elf

clean:
	rm -rf build
