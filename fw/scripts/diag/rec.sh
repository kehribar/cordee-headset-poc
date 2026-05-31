#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Record N seconds from the live USB-audio device (on the Pi), copy the WAV back
# to this laptop, and print a quick fingerprint.
#   usage: rec.sh [seconds] [out.wav]
# -----------------------------------------------------------------------------
. "$(dirname "$0")/env.sh"
DUR="${1:-3}"
OUT="${2:-/tmp/cap.wav}"

ssh "$PI" "arecord -D ${ALSA_DEV} -f ${CAP_FMT} -c ${CAP_CH} -r ${CAP_RATE} -d ${DUR} /tmp/_cap.wav" >/dev/null 2>&1
scp -q "$PI:/tmp/_cap.wav" "$OUT"
python3 "$(dirname "$0")/fp.py" "$OUT"
