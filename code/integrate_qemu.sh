#!/usr/bin/env bash
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

# Integrate the GRSPW SpaceWire device into a QEMU source tree and build it.
#
#   code/integrate_qemu.sh [QEMU_DIR]      (default: /workspace/qemu)
#
# - links code/leon3_grspw_v1 into QEMU as hw/misc/grspw (sources stay in this repo)
# - applies code/qemu-patches/*.patch (meson/Kconfig hooks + leon3 machine wiring)
# - rebuilds QEMU in QEMU_DIR/build
# The script is idempotent: it can be re-run after editing the device sources.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU_DIR="${1:-/workspace/qemu}"

[ -f "$QEMU_DIR/hw/sparc/leon3.c" ] || { echo "QEMU tree not found in $QEMU_DIR" >&2; exit 1; }

ln -sfn "$SCRIPT_DIR/leon3_grspw_v1" "$QEMU_DIR/hw/misc/grspw"

for patch in "$SCRIPT_DIR"/qemu-patches/*.patch; do
    if git -C "$QEMU_DIR" apply --reverse --check "$patch" 2>/dev/null; then
        echo "already applied: $(basename "$patch")"
    else
        git -C "$QEMU_DIR" apply "$patch"
        echo "applied: $(basename "$patch")"
    fi
done

make -C "$QEMU_DIR/build" -j"$(nproc)"
