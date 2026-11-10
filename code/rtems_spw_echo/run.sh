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

# Runs device_simu and the RTEMS application in QEMU, and checks the result.
# QEMU returns 0 even when RTEMS stops on an error, hence the console check.
#
#   run.sh EXECUTABLE DEVICE_SIMU QEMU RAM_MB
set -uo pipefail

app=$1 device_simu=$2 qemu=$3 ram_mb=$4
timeout_seconds=20

log=$(mktemp)

remove_queues() {
    rm -f /dev/mqueue/spwq /dev/mqueue/devq
}

# Stale messages from a previous run would be taken for acknowledges.
remove_queues

"$device_simu" &
simu_pid=$!
trap 'kill "$simu_pid" 2>/dev/null; remove_queues; rm -f "$log"' EXIT

timeout -k 1 "$timeout_seconds" "$qemu" -M leon3_generic -m "$ram_mb" \
    -display none -serial stdio -no-reboot -kernel "$app" 2>&1 |
    grep --line-buffered -v '^trace debug' | tee "$log"

if grep -q '^SPW ECHO OK' "$log"; then
    exit 0
fi

echo "SpaceWire echo failed" >&2
exit 1
