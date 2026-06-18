# file: Containerfile
#
# Builds an image with: host x86_64 freestanding gcc, aarch64 and
# riscv64 cross compilers, all three QEMU system emulators, make,
# binutils (objcopy) and groff (to read the man page).
#
# Build:
# $ podman build -t osdev -f Containerfile .
# See INSTALL for podman usage.

FROM docker.io/library/ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        binutils \
        gcc-aarch64-linux-gnu \
        gcc-riscv64-linux-gnu \
        qemu-system-x86 \
        qemu-system-arm \
        qemu-system-misc \
        groff-base \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
