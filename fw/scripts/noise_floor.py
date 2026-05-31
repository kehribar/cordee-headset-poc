#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Characterise the BROADBAND noise floor (ignore the comb teeth): median-filter
# the PSD to strip the narrow comb spikes, then look at spectral shape and
# integrated noise in voice / full bands. Tells us white vs pink vs HF-rising.
# -----------------------------------------------------------------------------
import wave, numpy as np
from scipy import signal, ndimage
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt

CAPS = [
    ("baseline",        "maxgain_30s.wav",     "tab:red"),
    ("CIC lowpass",     "cic_3khz.wav",        "tab:green"),
    ("notch 400Hz",     "notch_3khz_w400.wav", "tab:orange"),
]
FS = 48000

def load(p):
    with wave.open(p, "rb") as w:
        raw = w.readframes(w.getnframes())
    b = np.frombuffer(raw, np.uint8).reshape(-1, 3).astype(np.uint32)
    v = (b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)).astype(np.int32)
    v = np.where(v & 0x800000, v - (1 << 24), v)
    return v.astype(np.float64) / 2**23

def band_rms_dbfs(x, lo, hi):
    sos = signal.butter(6, [lo, hi], btype="band", fs=FS, output="sos")
    y = signal.sosfilt(sos, x)
    return 20 * np.log10(np.sqrt(np.mean(y**2)) + 1e-30)

fig, ax = plt.subplots(figsize=(13, 6))
print(f"{'capture':>14} {'floor@1k':>9} {'floor@5k':>9} {'slope dB/oct':>13} "
      f"{'rms 100-3k':>11} {'rms 3-8k':>9} {'rms full':>9}")
for name, f, c in CAPS:
    x = load(f)
    fr, p = signal.welch(x, fs=FS, window="hann", nperseg=16384, noverlap=8192)
    pdb = 10 * np.log10(p + 1e-30)
    # median filter strips the narrow comb spikes -> broadband floor only
    floor = ndimage.median_filter(pdb, size=151)
    ax.plot(fr / 1000, floor, color=c, lw=1.3, label=f"{name} (broadband floor)")
    ax.plot(fr / 1000, pdb, color=c, lw=0.4, alpha=0.25)

    def at(fhz): return floor[np.argmin(np.abs(fr - fhz))]
    slope = (at(8000) - at(1000)) / np.log2(8000 / 1000)
    print(f"{name:>14} {at(1000):>9.1f} {at(5000):>9.1f} {slope:>13.1f} "
          f"{band_rms_dbfs(x,100,3000):>11.1f} {band_rms_dbfs(x,3000,8000):>9.1f} "
          f"{band_rms_dbfs(x,20,23000):>9.1f}")

ax.set_xlim(0, FS/2000); ax.set_xlabel("kHz"); ax.set_ylabel("PSD (dB)")
ax.set_title("Broadband noise floor (bold = comb-stripped median; faint = raw PSD)")
ax.grid(alpha=0.3); ax.legend()
fig.tight_layout(); fig.savefig("noise_floor.png", dpi=120)
print("\n(rms values are dBFS in the band; slope <0 = falling toward HF (pink-ish),"
      " ~0 = white, >0 = rising toward HF)")
print("wrote noise_floor.png")
