/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

/*
 * ELF entry point used when booting with "qemu-system-sparc -kernel".
 *
 * The RTEMS sparc/leon3 start code reads the end of usable RAM from %sp,
 * which a real boot loader (mkprom, GRMON) sets. The small boot code QEMU
 * generates for -kernel leaves %sp at 0, which yields an empty memory area:
 * the AMBA scan then fails and the BSP stops with
 * LEON3_FATAL_NO_IRQMP_CONTROLLER before any console output.
 *
 * RAM_END is provided by the Makefile and must match QEMU's -m option.
 */

        .section .text.qemu_boot, "ax"
        .global  qemu_boot
        .type    qemu_boot, #function
qemu_boot:
        set     RAM_END, %sp
        set     start, %g1
        jmp     %g1
         nop
        .size   qemu_boot, . - qemu_boot
