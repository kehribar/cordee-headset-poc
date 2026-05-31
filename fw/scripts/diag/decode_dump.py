#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Decode a "[dump]" block from the serial console into a per-frame view.
#
# The diagnostic firmware can xprintf the raw captured DMA buffer as hex words.
# With the all-slot PIO (in pins,1 in every slot) there are 4 words per LRCK
# frame: slot1 slot2 slot3 slot4 = ch1 ch3 ch2 ch4. This reshapes the words by
# 4 and shows which slots actually carry data each frame.
#
# NOTE: the all-slot PIO's reshape phase is not frame-aligned (DMA can start
# mid-frame), so column<->slot mapping has an unknown rotation and the apparent
# period can alias. Single-slot captures (only one slot does `in pins,1`) are
# the reliable cross-check.
#   usage: pbpaste | decode_dump.py     OR    decode_dump.py serial.log
# -----------------------------------------------------------------------------
import sys, re

text = open(sys.argv[1]).read() if len(sys.argv) > 1 else sys.stdin.read()
words = [int(w, 16) for w in re.findall(r'\b[0-9A-Fa-f]{8}\b', text)]


def s24(w):                                  # firmware streams bits[31:8]
    v = (w >> 8) & 0xFFFFFF
    return v - (1 << 24) if v & 0x800000 else v


print(f"{len(words)} words")
for f in range(0, len(words) // 4 * 4, 4):
    row = words[f:f + 4]
    flags = ''.join('X' if w else '.' for w in row)
    print(f"frame {f//4:3d} [{flags}] " + " ".join(f"{w:08X}" for w in row))
