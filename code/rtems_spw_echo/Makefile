# SPDX-License-Identifier: GPL-2.0-or-later
#################################################################################
#                                                                               #
#     ▀▄   ▄▀                                                                   #
#    ▄█▀███▀█▄    Created on: Nov 6, 2026                                       #
#   █▀███████▀█   Author: Stephane GONZALEZ                                     #
#   █ █▀▀▀▀▀█ █                                                                 #
#      ▀▀ ▀▀                                                                    #
#                                                                               #
#  Copyright (C) 2015-2026 Stephane GONZALEZ                                    #
#  Released under the GNU GPL v2 or later, see LICENSE                          #
#################################################################################

# Builds the SpaceWire echo application for the RTEMS sparc/leon3 BSP and runs
# it with device_simu at the other end of the link.
#
# QEMU must contain the GRSPW device: see code/integrate_qemu.sh.

RTEMS_PREFIX ?= /opt/rtems/6
TARGET       ?= sparc-rtems6
BSP          ?= leon3
QEMU         ?= /workspace/qemu/build/qemu-system-sparc
RAM_MB       ?= 64

PKG := PKG_CONFIG_PATH=$(RTEMS_PREFIX)/lib/pkgconfig pkg-config $(TARGET)-$(BSP)
CC  := $(RTEMS_PREFIX)/bin/$(TARGET)-gcc

CFLAGS  := $(shell $(PKG) --cflags) -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections
ASFLAGS := $(shell $(PKG) --cflags) -g -DRAM_END='(0x40000000 + $(RAM_MB) * 1024 * 1024)'
# Start from qemu_boot.S instead of the BSP's `start`: see the comment in that file.
LDFLAGS := $(shell $(PKG) --libs) -Wl,-e,qemu_boot

BOOT_SRC := ../rtems_hello/qemu_boot.S
APP      := build/spw_echo.exe
SIMU_DIR := ../device_simu

all: $(APP)

$(APP): build/init.o build/qemu_boot.o
	$(CC) $^ $(LDFLAGS) -o $@

build/init.o: init.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/qemu_boot.o: $(BOOT_SRC) Makefile | build
	$(CC) $(ASFLAGS) -c $< -o $@

build:
	mkdir -p $@

run: all
	$(MAKE) -C $(SIMU_DIR)
	./run.sh $(APP) $(SIMU_DIR)/build/device_simu $(QEMU) $(RAM_MB)

clean:
	rm -rf build

.PHONY: all run clean
