#################################################################################
#                                                                               #
#     ▀▄   ▄▀                                                                   #
#    ▄█▀███▀█▄    Created on: Nov 6, 2026                                       #
#   █▀███████▀█   Author: Stephane                                              #
#   █ █▀▀▀▀▀█ █                                                                 #
#      ▀▀ ▀▀                                                                    #
#                                                                               #
#  Copyright 2015 Makefile                                                      #
#  Released under the "CeCILL-C" license                                        #
#  http://www.cecill.info                                                       #
#################################################################################

# Builds the Hello World application for the RTEMS sparc/leon3 BSP
# and runs it on QEMU's leon3_generic machine.

RTEMS_PREFIX ?= /opt/rtems/6
TARGET       ?= sparc-rtems6
BSP          ?= leon3
QEMU         ?= qemu-system-sparc
RAM_MB       ?= 64

PKG := PKG_CONFIG_PATH=$(RTEMS_PREFIX)/lib/pkgconfig pkg-config $(TARGET)-$(BSP)
CC  := $(RTEMS_PREFIX)/bin/$(TARGET)-gcc

CFLAGS  := $(shell $(PKG) --cflags) -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections
ASFLAGS := $(shell $(PKG) --cflags) -g -DRAM_END='(0x40000000 + $(RAM_MB) * 1024 * 1024)'
# Start from qemu_boot.S instead of the BSP's `start`: see the comment in that file.
LDFLAGS := $(shell $(PKG) --libs) -Wl,-e,qemu_boot

APP := build/hello.exe

all: $(APP)

$(APP): build/init.o build/qemu_boot.o
	$(CC) $^ $(LDFLAGS) -o $@

build/init.o: init.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/qemu_boot.o: qemu_boot.S Makefile | build
	$(CC) $(ASFLAGS) -c $< -o $@

build:
	mkdir -p $@

run: $(APP)
	$(QEMU) -M leon3_generic -m $(RAM_MB) -display none -serial stdio -no-reboot -kernel $(APP)

clean:
	rm -rf build

.PHONY: all run clean
