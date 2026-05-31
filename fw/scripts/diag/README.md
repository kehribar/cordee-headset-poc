# Diagnostic tooling: build / flash / capture loop

Scripts used to investigate the MIC4 TDM-decode bug (see
`src/mic/notes.md` -> "TDM DECODE FINDING"). They automate the
edit -> build -> flash -> record/inspect cycle against the remote rig.

## Hardware / network topology

```
  this laptop (build host)
      |  ssh + scp + gdb-multiarch (Tailscale)
      v
  Pi  "pi5-a"  (pi@pi5-a.tail302a2.ts.net, IP 100.84.168.69)
      |-- picoprobe / CMSIS-DAP --> RP2350 SWD     (OpenOCD GDB server :3333)
      |-- USB  ----------------- --> RP2350 USB-audio "Cordee Headset" (UAC)
      |-- USB-serial ----------- --> RP2350 PIO-UART console /dev/ttyACM0 @1Mbaud
```

- We **build** the firmware on the laptop (`make` in `fw/build`).
- We **flash** over the network to an **OpenOCD GDB server running on the Pi**
  (`100.84.168.69:3333`), which drives the RP2350 via the picoprobe.
  - Start the server on the Pi with `scripts/picoprobe_server_cmsis.sh`
    (`ssh pi@pi5-a.tail302a2.ts.net` first).
- The RP2350's **USB-audio** output goes to the Pi; we capture it there with
  `arecord` and `scp` the WAV back.
- The firmware's **xprintf console** (boot banner, i2c scan, chip id, any debug
  dumps) comes out on the Pi's `/dev/ttyACM0` at **1000000 baud**.

All host/port/device values live in `env.sh`; override via env vars if needed.

## Scripts

| script            | what it does |
|-------------------|--------------|
| `env.sh`          | shared config (paths, OpenOCD host, Pi, ALSA dev, serial dev). Sourced by the others. |
| `flash.sh`        | `make` + flash to OpenOCD, reset-run, **detach and return** (loop-friendly, unlike the interactive `picoprobe_gdb_linux.sh`). |
| `rec.sh [s] [out]`| `arecord` N seconds on the Pi, `scp` back, print fingerprint. |
| `serial.sh [s]`   | stream the MCU serial console from the Pi to stdout + `/tmp/serial.log`. |
| `fp.py file.wav`  | WAV fingerprint: non-zero fraction, hot phases (mod 16), RMS/peak, dominant freqs. |
| `decode_dump.py`  | turn a serial `[dump]` hex block into a per-frame slot view. |

## Typical loops

Plain build/flash + listen-fingerprint:

```bash
cd fw/scripts/diag
./flash.sh
./rec.sh 3 /tmp/test.wav      # -> nz=... hot16=... top freqs
```

Capture the boot log + register readback (flash resets the MCU, so start the
serial reader first, in the background):

```bash
./serial.sh 20 &              # background; logs to /tmp/serial.log
sleep 1
./flash.sh                    # reset triggers the boot banner on serial
sleep 4
cat /tmp/serial.log
```

Inspect the raw captured TDM frame (needs the diagnostic firmware that xprintf's
the DMA buffer; see notes.md for what the dump means):

```bash
./decode_dump.py /tmp/serial.log
```

## Healthy vs broken fingerprint

- **Healthy mic stream:** `fp.py` shows `nz~1.0`, no hot-slot structure.
- **The bug:** `nz=0.1250`, `hot16=[a, a+1]` (only 2 of every 16 samples
  non-zero) and dominant freqs at Fs/16 + harmonics (3000/6000 Hz @ 48 k) — the
  zero-stuffing "comb". A synthetic ramp in `mic_i2s_process` comes through at
  `nz~1.0`, which is how we proved the USB/host path is clean and the defect is
  in the codec->PIO->DMA capture.
```
