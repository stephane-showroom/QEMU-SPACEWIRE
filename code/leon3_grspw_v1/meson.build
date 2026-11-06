#################################################################################
#                                                                               #
#     ▀▄   ▄▀                                                                   #
#    ▄█▀███▀█▄    Created on: Nov 6, 2026                                       #
#   █▀███████▀█   Author: Stephane                                              #
#   █ █▀▀▀▀▀█ █                                                                 #
#      ▀▀ ▀▀                                                                    #
#                                                                               #
#  Copyright 2015 meson.build                                                   #
#  Released under the "CeCILL-C" license                                        #
#  http://www.cecill.info                                                       #
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
