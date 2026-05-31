# DAC / speaker notes

Amp: **NS4168** (see `doc/NS4168.pdf`) — mono I2S class-D, 2.5 W @ VDD=5 V /
RL=4 Ω. No I2C, no volume register: **fixed gain**, full-scale digital in →
full output. Pins: BCLK/LRCK/DOUT on GPIO 26/27/28, **enable on GPIO29** (driven
high in `hardware_init`). CTRL high = right channel (we duplicate mono to L+R,
so channel select doesn't matter).

# "Speaker output very low" investigation — NOT a bit-alignment bug

Verified the DAC PIO timing against the NS4168 datasheet (don't trust the .pio
comments — traced the instructions):

- NS4168 wants **standard I2S = "BCLK delay one clock"** (MSB 1 BCLK after the
  LRCK edge), MSB-first, two's complement (§10.1–10.2).
- Our PIO: LRCK changes on a BCLK falling edge during the old channel's 32nd
  bit; the new channel's MSB is the next bit → sampled exactly 1 BCLK after the
  WS edge. **That is standard I2S, correctly aligned.** (Not left-justified.)
- BCLK = 64·Fs = 3.072 MHz @ 48 kHz is a supported NS4168 rate (Table 2).
- Data packing `(uint32_t)pcm << 16` = full-scale, MSB-justified in the 32-bit
  slot. No software volume anywhere.

So the digital path is correct and full-scale; low volume is **not** firmware
alignment. Likely real causes, in order:
1. **VDD on the NS4168** — needs ~5 V for 2.5 W; at 3.3 V output power ≈ 44 %.
2. Host mixer / source level.
3. Adaptive-clock FIFO drift (no feedback EP) → `usb_audio_read_spk` zero-fills
   underruns → choppy / lower average level.
4. Speaker impedance/size.

Isolation test (use again if needed): temporarily emit a full-scale on-device
440 Hz sine in `dac_i2s_process` (bypassing the USB FIFO) for the first 10 s.
Loud → digital path fine, chase VDD/host; quiet → hardware (VDD/amp/speaker).
[Result pending listen as of 2026-05-31.]

# Speaker buffering (dac_i2s_process)
Reduced to a **startup pre-roll only**: wait ~10 ms of buffered audio before the
first playback, then stay running and let `usb_audio_read_spk` zero-fill any
momentary shortfall. Removed the old re-prime-on-underrun, which forced a ~10 ms
silence on every underrun under the unsynced/adaptive clock (audible periodic
dropouts, made playback seem quiet/choppy). A real fix for drift would be the
feedback EP (see ../usb/notes.md).
