#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Shared config for the build / flash / capture loop.
# Source this from the other scripts:  . "$(dirname "$0")/env.sh"
# Override any of these with environment variables if your setup differs.
# -----------------------------------------------------------------------------

# Repo's firmware build directory (where CMake/make output the .elf).
FW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$FW_DIR/build}"
ELF="${ELF:-cordee-heatset.elf}"

# OpenOCD GDB server. Runs on the Pi (pi5-a) attached to the picoprobe/CMSIS-DAP.
# Start it there with scripts/picoprobe_server_cmsis.sh (or the systemd unit).
OPENOCD_HOST="${OPENOCD_HOST:-100.84.168.69}"
OPENOCD_PORT="${OPENOCD_PORT:-3333}"

# The Pi that the RP2350 USB-audio device + the picoprobe are plugged into.
PI="${PI:-pi@pi5-a.tail302a2.ts.net}"

# ALSA capture device on the Pi. The UAC device enumerates as "Cordee Headset".
#   (verify with:  ssh $PI arecord -l )
ALSA_DEV="${ALSA_DEV:-hw:CARD=Headset,DEV=0}"
CAP_RATE="${CAP_RATE:-48000}"
CAP_FMT="${CAP_FMT:-S24_3LE}"          # 24-bit packed, matches the mic IN endpoint
CAP_CH="${CAP_CH:-1}"

# MCU serial console (firmware xprintf, PIO-UART -> USB-serial on the Pi).
SERIAL_DEV="${SERIAL_DEV:-/dev/ttyACM0}"
SERIAL_BAUD="${SERIAL_BAUD:-1000000}"
