/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane                                              *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright 2015 qemu_boot.S                                                   *
 * Released under the "CeCILL-C" license                                        *
 * http://www.cecill.info                                                       *
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
