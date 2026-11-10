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

# GRSPW SpaceWire codec for the LEON3 (GRLIB) machine.
# This directory is linked into the QEMU tree as hw/misc/grspw
# by code/integrate_qemu.sh.
system_ss.add(when: 'CONFIG_GRLIB_GRSPW', if_true: files(
  'grlib_apbgrspw.c',
  'grlib_comgrspw.c',
  'grlib_coregrspw.c',
  'grlib_loggrspw.c',
  'grlib_mqgrspw.c',
  'grlib_reggrspw.c',
  'grlib_rmapgrspw.c',
  'grlib_utilgrspw.c',
))
