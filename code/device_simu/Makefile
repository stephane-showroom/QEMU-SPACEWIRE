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

# Builds the SpaceWire loopback simulator for the host.

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra
CFLAGS  += -I../leon3_grspw_v1
LDLIBS  := -lrt

APP := build/device_simu

all: $(APP)

$(APP): device_simu.c ../leon3_grspw_v1/grlib_mqgrspw.h ../leon3_grspw_v1/grlib_typesgrspw.h | build
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

build:
	mkdir -p $@

run: $(APP)
	./$(APP)

clean:
	rm -rf build

.PHONY: all run clean
