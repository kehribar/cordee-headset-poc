# -----------------------------------------------------------------------------
# Erase the entire QSPI flash on the RP2350 to recover a chip wedged in the
# bootrom (symptoms: cores halted at pc 0x88 with a garbage msp, OpenOCD
# "Failed to read memory" / "Could not allocate stack for flash programming
# code"). A corrupt/half-written image stops the bootrom from handing off;
# erasing flash forces it back into a clean USB-bootloader state.
#
# Usage:
#   1. Stop the GDB server (picoprobe_server_cmsis.sh) -- the probe can only be
#      owned by one OpenOCD instance.
#   2. Put the board in a clean state: hold BOOTSEL while replugging USB.
#   3. Run this script from the scripts/ dir.
#
# (If picotool is installed, "picotool erase && picotool reboot" with the board
#  in BOOTSEL does the same thing.)
# -----------------------------------------------------------------------------
openocd \
  -f interface/cmsis-dap.cfg \
  -f target/rp2350.cfg \
  -c "adapter speed 2000" \
  -s tcl \
  -c "init; reset halt; flash erase_sector 0 0 last; shutdown"
