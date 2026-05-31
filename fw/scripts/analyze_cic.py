#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Compare the pre-filter baseline capture against the 3 kHz CIC capture:
#   - overlaid Welch PSD (full band + comb zoom)
#   - STFT spectrogram of each
#   - per-tooth prominence table for the Fs/16 comb (3,6,9,... kHz)
# -----------------------------------------------------------------------------
import sys
import wave
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import signal

BASE = sys.argv[1] if len(sys.argv) > 1 else "maxgain_30s.wav"
FILT = sys.argv[2] if len(sys.argv) > 2 else "cic_3khz.wav"
OUT  = sys.argv[3] if len(sys.argv) > 3 else "cic_compare.png"


def load_wav(path):
    """Return (float64 samples in [-1,1], fs). Handles 16/24/32-bit mono PCM."""
    with wave.open(path, "rb") as w:
        ch, sw, fs, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if sw == 2:
        x = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 2**15
    elif sw == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.uint32)
        v = (b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)).astype(np.int32)
        v = np.where(v & 0x800000, v - (1 << 24), v)          # sign-extend 24-bit
        x = v.astype(np.float64) / 2**23
    elif sw == 4:
        x = np.frombuffer(raw, dtype="<i4").astype(np.float64) / 2**31
    else:
        raise ValueError(f"unsupported sample width {sw}")
    if ch > 1:
        x = x.reshape(-1, ch)[:, 0]
    return x, fs


def psd_db(x, fs, nperseg=16384):
    f, p = signal.welch(x, fs=fs, window="hann", nperseg=nperseg,
                        noverlap=nperseg // 2, detrend="constant")
    return f, 10.0 * np.log10(p + 1e-30)


xb, fsb = load_wav(BASE)
xf, fsf = load_wav(FILT)
print(f"baseline : {BASE}  fs={fsb}  {len(xb)/fsb:.2f}s  rms={np.sqrt(np.mean(xb**2)):.3e}")
print(f"filtered : {FILT}  fs={fsf}  {len(xf)/fsf:.2f}s  rms={np.sqrt(np.mean(xf**2)):.3e}")

fb, pb = psd_db(xb, fsb)
ff, pf = psd_db(xf, fsf)

# --- per-tooth prominence (peak minus local floor, calibration-independent) ---
combs = [3000 * k for k in range(1, 8)]            # 3,6,9,...,21 kHz
def prominence(f, p, f0, bw=120.0, guard=350.0):
    peak = p[(f >= f0 - bw) & (f <= f0 + bw)].max()
    side = p[((f >= f0 - guard) & (f <= f0 - guard + 200)) |
             ((f >= f0 + guard - 200) & (f <= f0 + guard))]
    return peak, peak - np.median(side)

print("\nFs/16 comb teeth — prominence above local floor (dB):")
print(f"{'freq':>7} {'base':>8} {'cic':>8} {'change':>8}")
rows = []
for f0 in combs:
    _, pb0 = prominence(fb, pb, f0)
    _, pf0 = prominence(ff, pf, f0)
    rows.append((f0, pb0, pf0, pf0 - pb0))
    print(f"{f0/1000:>6.0f}k {pb0:>8.1f} {pf0:>8.1f} {pf0-pb0:>+8.1f}")

# ----------------------------------------------------------------------------
fig = plt.figure(figsize=(14, 11))

ax1 = fig.add_subplot(3, 1, 1)
ax1.plot(fb / 1000, pb, lw=0.8, color="tab:red", label=f"baseline ({BASE})")
ax1.plot(ff / 1000, pf, lw=0.8, color="tab:blue", label=f"3 kHz CIC ({FILT})")
for f0 in combs:
    ax1.axvline(f0 / 1000, color="k", ls=":", lw=0.6, alpha=0.5)
ax1.set_xlim(0, fsb / 2000)
ax1.set_xlabel("kHz"); ax1.set_ylabel("PSD (dB)")
ax1.set_title("Welch PSD — full band (dotted = Fs/16 comb teeth)")
ax1.legend(loc="upper right"); ax1.grid(alpha=0.3)

ax2 = fig.add_subplot(3, 1, 2)
ax2.plot(fb / 1000, pb, lw=1.0, color="tab:red", label="baseline")
ax2.plot(ff / 1000, pf, lw=1.0, color="tab:blue", label="3 kHz CIC")
for f0 in combs:
    ax2.axvline(f0 / 1000, color="k", ls=":", lw=0.6, alpha=0.5)
ax2.set_xlim(0, 13)
ax2.set_xlabel("kHz"); ax2.set_ylabel("PSD (dB)")
ax2.set_title("Welch PSD — comb zoom (0–13 kHz)")
ax2.legend(loc="upper right"); ax2.grid(alpha=0.3)

vmax = pb.max()
for i, (x, fs, name) in enumerate([(xb, fsb, "baseline"), (xf, fsf, "3 kHz CIC")]):
    ax = fig.add_subplot(3, 2, 5 + i)
    f, t, Sxx = signal.spectrogram(x, fs=fs, window="hann",
                                  nperseg=4096, noverlap=3072)
    ax.pcolormesh(t, f / 1000, 10 * np.log10(Sxx + 1e-30),
                  shading="gouraud", vmin=vmax - 90, vmax=vmax, cmap="magma")
    ax.set_ylim(0, fs / 2000)
    ax.set_xlabel("s"); ax.set_ylabel("kHz"); ax.set_title(f"STFT — {name}")

fig.tight_layout()
fig.savefig(OUT, dpi=110)
print(f"\nwrote {OUT}")
