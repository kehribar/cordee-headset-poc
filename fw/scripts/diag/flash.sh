#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Build the firmware and flash it to the remote OpenOCD GDB server, then LEAVE
# THE TARGET RUNNING and return (non-interactive). Unlike picoprobe_gdb_linux.sh
# this does not stay attached with "continue" -- it loads, resets, detaches and
# quits, so it is usable inside an automated build/flash/record loop.
# -----------------------------------------------------------------------------
set -e
. "$(dirname "$0")/env.sh"

cd "$BUILD_DIR"
touch ../src/main.c                      # force a relink so the .elf is fresh
make -j32 >/tmp/build.log 2>&1 || { echo BUILD_FAIL; tail -20 /tmp/build.log; exit 1; }

gdb-multiarch -batch \
  -ex "set confirm off" \
  -ex "target remote ${OPENOCD_HOST}:${OPENOCD_PORT}" \
  -ex "file ${ELF}" \
  -ex "load" \
  -ex "monitor reset run" \
  -ex "detach" \
  -ex "quit" >/tmp/flash.log 2>&1

echo "FLASH_DONE"
grep -iE "transfer rate|error" /tmp/flash.log | tail -4
