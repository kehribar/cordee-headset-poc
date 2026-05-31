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

# MIC NOISE INVESTIGATION: Fs/16 COMB

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
