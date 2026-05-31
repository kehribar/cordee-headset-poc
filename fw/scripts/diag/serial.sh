#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Capture the MCU serial console (firmware xprintf output) from the Pi.
# The firmware logs over a PIO-UART on GPIO10 @ 1 Mbaud, which appears as a
# USB-serial adapter (/dev/ttyACM0) on the Pi.
#
# Typical use: run this in the background, THEN flash (flash.sh resets the MCU,
# so the boot banner + [es7210] chip id + any debug dumps appear here).
#   usage: serial.sh [seconds]   (writes to /tmp/serial.log and stdout)
# -----------------------------------------------------------------------------
. "$(dirname "$0")/env.sh"
DUR="${1:-20}"

ssh "$PI" "stty -F ${SERIAL_DEV} ${SERIAL_BAUD} raw -echo; timeout ${DUR} cat ${SERIAL_DEV}" \
  | tee /tmp/serial.log
