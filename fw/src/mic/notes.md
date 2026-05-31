# PINOUT
GPIO0: MIC_INT
GPIO1: MIC_STDOUT
GPIO2: MIC_LRCK
GPIO3: MIC_SCLK
GPIO4: MIC_MCLK
GPIO5: MIC_SCL
GPIO6: MIC_SDA

# CHIP
ES7210

# NOTES
ONLY MIC4 IS POPULATED

# WHY TDM IS REQUIRED (not optional here)
We need MIC4, and only ONE serial-data pin is wired: SDOUT1/TDMOUT (GPIO1).
On the ES7210 the data routing in NON-TDM mode (reg 0x12 SDOUT_MODE=00) is:
  ADC12 (MIC1/MIC2) -> SDOUT1
  ADC34 (MIC3/MIC4) -> SDOUT2
MIC4 is part of the ADC34 pair, so in plain 2-channel I2S it comes out on
**SDOUT2** -- which is NOT connected on this board. We would only ever see
MIC1/MIC2 (no mic) on SDOUT1.
To get MIC4 onto the single wired pin (SDOUT1) we MUST use a TDM mode
(reg 0x12 SDOUT_MODE = 10 = "TDM I2S/LJ"), which multiplexes all 4 ADC channels
onto SDOUT1 and lets the PIO pick the ch4 time-slot. Hence TDM is mandatory,
not a stylistic choice -- the "switch to 2-channel I2S" idea below cannot reach
MIC4 over the available wiring.

# MIC NOISE INVESTIGATION: Fs/16 COMB

> **UPDATE 2026-05-31 — root cause is most likely TDM DECODING, not analog
> coupling.** The "Fs/16 comb" is almost certainly the spectral signature of a
> TDM framing / decode bug, NOT LRCK/BCLK electrical coupling into the analog
> front-end. The electrical-coupling conclusion below is kept for history but is
> now believed to be WRONG. See "TDM DECODE FINDING" section at the bottom.

A persistent tonal comb sits on the MIC4 capture. Characterised 2026-05-30.

## Symptoms
- Comb fundamental at **Fs/16** (3000 Hz @ 48 kHz), harmonics at 2x/3x/4x.
- Each comb tooth has **+/-200 Hz sidebands**; plus standalone mains lines
  (100/200/300 Hz).
- Sits ~13 dB below the broadband noise floor (not dominant, but structured).

## Localisation tests (fixed 37.5 dB PGA, "silent" room)
- **Scales with PGA gain** (+31.5 dB across 0->37.5 dB, same as broadband)
  => input-referred, enters at/before the PGA analog input.
- **DAC + speaker disabled**: no change => not the on-board playback path.
- **MIC4 disconnected from PGA (SELMIC=0)**: comb barely moves (-1.8 dB), but
  the 100 Hz mains line drops -6.5 dB => mains hum enters via the mic pin, the
  comb does NOT.
- **MIC34 bias 2.87V -> 2.18V**: no change => not the bias voltage level.
- **MCLK 12.288 -> 19.2 MHz** (256x->400x, codec dividers retuned): comb does
  NOT move (stays at 3000 Hz) => NOT MCLK feedthrough.
- **Fs 48 -> 60 kHz** (end-to-end, USB rate + EP buffers resized): comb moves
  3000 -> 3750 Hz, i.e. exactly x1.25 = Fs/16. Sidebands stay at +/-200 Hz and
  300 Hz line stays put => those are a FIXED ~50/200 Hz mains source.

## Conclusion
The dominant tonal interference is an **Fs/16 comb generated in the LRCK/frame-
clock (I2S/TDM sync) domain** the RP2350 feeds the codec. It couples into the
ES7210 analog front-end (hence input-referred, PGA-amplified), and is amplitude-
modulated by fixed mains ripple (the +/-200 Hz sidebands + standalone hum).
It is NOT MCLK feedthrough, NOT acoustic, NOT the speaker, NOT the mic capsule.

Best mitigated in hardware (LRCK/BCLK routing away from the analog input,
analog-supply/ground isolation, AVDD decoupling at the ES7210), not firmware.

## Mitigations tried (measured, "prominence" = peak above local floor)
- **Mic LRCK/BCLK pins -> 2 mA + slow slew** (mic_i2s.c): comb CARRIERS drop
  ~5 dB (3 kHz: 34 -> 29 dB prominence; 6/9/12 kHz similar). KEPT in firmware.
  2 mA is the RP2350 pad floor; slow slew is the only further knob.
- **DAC I2S pins -> 2 mA + slow slew** (dac_i2s.c): no measurable effect on the
  mic comb (confirms DAC path is not the aggressor). Kept anyway as EMI hygiene.
- **SCLK invert** (MODE_CFG 0x08 bit3): no effect on the comb (just re-phases
  the sampling edge; carriers even slightly higher). Not kept.
- The +/-200 Hz **sidebands are immune to drive strength** (they are the mains-
  modulation component) and remain ~34 dB prominence -> a separate hum-path
  problem (the 100 Hz mains enters via the mic input wiring, see SELMIC test).

## Codec-internal levers (ES7210)
- No notch / parametric EQ to target Fs/16. (Reg 0x08 bit2 EQ_OFF exists but
  needs coeff programming; default EQ is effectively flat.)
- HPF regs (0x20-0x23) only affect DC/LF -> help the 100 Hz hum, not 3 kHz.
- Untested-but-promising: 64*Fs 2-channel I2S frame instead of 128*Fs 4-slot
  TDM (only MIC4 used) -> halves BCLK switching activity near the analog input.

## Notes for reproducing the Fs test
- Changing Fs requires keeping main.c `FS` == usb_descriptors.h
  `AUDIO_SAMPLE_RATE`, and the EP sizes must fit the new rate:
  max samples/frame = (Fs+999)/1000 + 1, EP bytes = that * subslot * ch.
  Mic EP (24-bit): 60k -> 183 B; Spk EP (16-bit): 60k -> 122 B. Undersized EP
  IN buffer at higher Fs => xHCI "buffer overrun" flood on the mic IN endpoint.
- MCLK = 256*Fs (PIO square wave); MAINCLK reg 0xC1 is valid for any 256x Fs.

# TDM DECODE FINDING (2026-05-31)

The "Fs/16 comb" is most probably a **TDM framing / decode bug**, not analog
coupling. The captured MIC4 stream is structurally broken: it is mostly hard
zeros, and zero-stuffing a sparse signal is exactly what produces an Fs/16 comb
plus harmonics in the spectrum. Investigated by flashing diagnostic firmware to
the device and recording over USB on the Pi (UART console on /dev/ttyACM0).

## What the capture actually contains
- In the 48 kHz mono stream, only **2 of every 16 samples are non-zero**; the
  other 14 are EXACTLY 0x000000. Stable over a full 30 s recording, no drift.
- Holds at max PGA gain too -> NOT level dependent, it is structural.
- The two non-zero samples per period are two *different* consecutive values
  (look like the ADC34 pair, ch3+ch4) -> the codec appears to emit a TDM frame
  spanning **16 LRCK periods** with only ~2 channels driven, then idle.
- Pattern is identical whichever PIO slot we capture (tested slots 2/3/4 each
  individually -> all 2/16) -> SDOUT is idle 14/16 frames in *every* slot, so
  this is the codec's output, not a wrong-slot/PIO-alignment problem.

## What was RULED OUT (so it is not these)
- **USB / host path**: a synthetic ramp written in mic_i2s_process comes through
  perfectly continuous at 48 k (monotonic, no zero-stuffing). The corruption is
  upstream of USB, between codec -> PIO -> DMA.
- **Codec register config**: read back over I2C (streamed out via audio + via
  UART). Chip ID = 0x7210 (genuine). MAINCLK=0xC1, OSR=0x20, MODE=0x00 (slave),
  SDP1=0x81 (32-bit, LJ), SDP2=0x02 (SDOUT_MODE=10 = "TDM I2S/Left Justified"),
  MIC4 selected (M4G=0x1E), both pairs powered, clocks on. All match the
  Espressif es7210 driver + datasheet exactly for 12.288 MHz / 48 kHz.
- **Clocks**: MCLK=12.288 MHz (256fs), BCLK=6.144 MHz (128fs), LRCK=48 kHz — all
  verified from the clkdivs; ADC fs computes to 48 k. LRCK=48 k also confirmed by
  the ramp test (DMA/capture rate = 48 k).
- **PIO framing vs datasheet**: matches Fig 2f (TDM-LJ): LRCK-high=[Ch1][Ch3],
  LRCK-low=[Ch2][Ch4], so ch4=MIC4 is the 4th slot the PIO captures. Correct.

## Conclusion / why it points at TDM decode
Every static thing is correct, yet the ES7210 drives real data on SDOUT only
~1/8 of the LRCK frames, i.e. its effective TDM frame is ~16 LRCK long instead
of 1. That is a framing mismatch between what the chip emits and the single-LRCK
4-slot frame the PIO assumes. The "comb" the earlier analog investigation chased
is just the FFT of this 7/8 zero-stuffed signal. The earlier "scales with Fs"
observation is consistent with this (framing is locked to LRCK, not MCLK).

# RESOLVED (2026-05-31): reg 0x08 MODE_CFG was the bug

One register: **MODE_CFG (0x08) was 0x00; it must be 0x10** (the Espressif
es7210 value). Fix is in `es7210.c`. Capture is now continuous: nz=1.0, the
Fs/16 comb is gone (3/6/9 kHz peaks back down to the broadband floor), per-mod16
sample means are flat.

## How it was found (build/flash/record loop in scripts/diag)
Added a temporary all-slot capture PIO (`in pins,1` on every bit -> 4 words/LRCK)
plus a one-shot serial dump of the DMA buffer, and reshaped it. Ground truth:
- With the original config the chip drove real data in only ~3 of every **16**
  32-bit slots, the other 13 hard zero -> the chip was emitting a **16-slot /
  512-BCLK TDM frame** (the 4-chip / 16-channel *cascade* frame), NOT a 4-slot
  one. Our 4-slot / 128-BCLK PIO therefore lined up with ch4 only 2 of every 16
  LRCK periods. That 7/8 zero-stuffing is exactly the "Fs/16 comb".
- Sweeping MODE_CFG: 0x00 and 0x20 both kept the 16-slot frame; **0x10 collapsed
  it to a clean 4-slot frame**. (LRCK_RATE_MODE in 0x08 is documented as
  "SDOUT_MODE=11 only", but in practice it gates the slot count here too.)
- With 0x10 + LJ format (0x81), all four slots read nz~1.0; ch4 (mic4) is the
  4th slot — exactly what the existing `mic_i2s.pio` already captures. So the
  **PIO was correct all along**; no PIO change was needed.

Also added (matches Espressif `es7210_start`, kept as hygiene): a final
reg0x00 0x71->0x41 digital-engine reset AFTER all config writes.

## Build gotcha hit along the way
`mic_i2s.pio.h` is committed and generated into the *source* tree
(`pico_generate_pio_header ... OUTPUT_DIR src/mic`), which makes `make` drop the
regeneration rule as a self-dependency. Editing `mic_i2s.pio` does NOT rebuild
the header unless you `rm src/mic/mic_i2s.pio.h` first. Caught a whole slot
sweep that was silently using the stale header.

## Earlier hypotheses, now ruled out
DSP/PCM TDM and codec MASTER mode were the next ideas but proved unnecessary —
the slave-mode framing was fine once the slot count matched (0x08=0x10).
