#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Build locally, flash the RP2350 over gdb->openocd (running on pi5-a),
# then record 30 s of mic audio on pi5-a and copy the WAV back.
# Self-guarding: every step aborts the run if the previous one failed.
# -----------------------------------------------------------------------------
set -euo pipefail

REPO=/home/kehribar/GitRepo/cordee-headset-poc/fw
BUILD=$REPO/build
ELF=$BUILD/cordee-heatset.elf
OCD=100.84.168.69:3333
PI=pi@pi5-a.tail302a2.ts.net
WAVNAME=${1:-cic_3khz}
REMOTE_WAV=/tmp/$WAVNAME.wav
LOCAL_WAV=$REPO/$WAVNAME.wav
DUR=30

echo "=== [1/4] BUILD ($(date '+%H:%M:%S')) ==="
cd "$BUILD"
make -j32

echo "=== [2/4] FLASH via gdb -> openocd ($OCD) ==="
gdb-multiarch -batch \
  -ex "set confirm off" \
  -ex "target remote $OCD" \
  -ex "file $ELF" \
  -ex "load" \
  -ex "compare-sections" \
  -ex "monitor reset run" \
  -ex "detach" \
  -ex "quit"

echo "=== [3/4] wait for USB re-enumeration on Pi ==="
sleep 6
ssh -o ConnectTimeout=10 "$PI" 'arecord -l | grep -i headset' \
  || { echo "ERROR: Cordee Headset capture device not present after flash"; exit 1; }

echo "=== [4/4] RECORD ${DUR}s on Pi ==="
ssh -o ConnectTimeout=10 "$PI" \
  "arecord -D plughw:CARD=Headset,DEV=0 -f S24_3LE -c 1 -r 48000 -d $DUR $REMOTE_WAV && ls -l $REMOTE_WAV"

echo "=== copy WAV back ==="
scp "$PI:$REMOTE_WAV" "$LOCAL_WAV"
ls -l "$LOCAL_WAV"
echo "=== DONE ($(date '+%H:%M:%S')) ==="
