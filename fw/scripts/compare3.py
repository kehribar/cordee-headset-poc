#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# 3-way comparison: baseline vs 3 kHz CIC low-pass vs 3 kHz comb notch.
#   row 1: Welch PSD, full band
#   row 2: Welch PSD, comb zoom (0-13 kHz)
#   row 3: fine-resolution zoom on the 3 kHz and 6 kHz teeth (carrier + sidebands)
#   row 4: STFT spectrogram of each capture
# -----------------------------------------------------------------------------
import wave
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import signal

CAPS = [
    ("baseline",                "maxgain_30s.wav",     "tab:red"),
    ("3 kHz CIC lowpass",       "cic_3khz.wav",        "tab:green"),
    ("comb notch ~29 Hz (a=.97)",  "notch_3khz.wav",      "tab:blue"),
    ("comb notch ~400 Hz (a=.58)", "notch_3khz_w400.wav", "tab:orange"),
]
OUT = "cic_compare.png"
FS = 48000


def load(path):
    with wave.open(path, "rb") as w:
        raw = w.readframes(w.getnframes())
    b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.uint32)
    v = (b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)).astype(np.int32)
    v = np.where(v & 0x800000, v - (1 << 24), v)
    return v.astype(np.float64) / 2**23


sig = {name: (load(f), c) for name, f, c in CAPS}
combs = [3000 * k for k in range(1, 8)]

def psd(x, nps):
    f, p = signal.welch(x, fs=FS, window="hann", nperseg=nps, noverlap=nps // 2)
    return f, 10 * np.log10(p + 1e-30)

fig = plt.figure(figsize=(15, 16))

# --- row 1: full band ---
ax1 = fig.add_subplot(4, 1, 1)
for name, (x, c) in sig.items():
    f, p = psd(x, 16384)
    ax1.plot(f / 1000, p, lw=0.7, color=c, label=name)
for f0 in combs:
    ax1.axvline(f0 / 1000, color="k", ls=":", lw=0.5, alpha=0.4)
ax1.set_xlim(0, FS / 2000); ax1.set_xlabel("kHz"); ax1.set_ylabel("PSD (dB)")
ax1.set_title("Welch PSD - full band (dotted = Fs/16 comb teeth)")
ax1.legend(loc="upper right"); ax1.grid(alpha=0.3)

# --- row 2: comb zoom ---
ax2 = fig.add_subplot(4, 1, 2)
for name, (x, c) in sig.items():
    f, p = psd(x, 16384)
    ax2.plot(f / 1000, p, lw=0.9, color=c, label=name)
for f0 in combs:
    ax2.axvline(f0 / 1000, color="k", ls=":", lw=0.5, alpha=0.4)
ax2.set_xlim(0, 13); ax2.set_xlabel("kHz"); ax2.set_ylabel("PSD (dB)")
ax2.set_title("Welch PSD - comb zoom (0-13 kHz)")
ax2.legend(loc="upper right"); ax2.grid(alpha=0.3)

# --- row 3: fine zoom on individual teeth (carrier vs +/-200 Hz sidebands) ---
nps = 1 << 17  # ~0.37 Hz/bin
fine = {name: psd(x, nps) for name, (x, c) in sig.items()}
for col, fc in enumerate((3000, 6000)):
    ax = fig.add_subplot(4, 2, 5 + col)
    for name, (x, c) in sig.items():
        f, p = fine[name]
        m = (f >= fc - 500) & (f <= fc + 500)
        ax.plot(f[m], p[m], lw=1.0, color=c, label=name)
    ax.axvline(fc, color="k", ls=":", lw=0.8)
    ax.axvline(fc - 200, color="gray", ls=":", lw=0.6)
    ax.axvline(fc + 200, color="gray", ls=":", lw=0.6)
    ax.set_title(f"{fc} Hz tooth (dotted: carrier & +/-200 Hz mains sidebands)")
    ax.set_xlabel("Hz"); ax.set_ylabel("PSD (dB)"); ax.grid(alpha=0.3)
    ax.legend(fontsize=8)

# --- row 4: STFT of each ---
vmax = max(psd(x, 16384)[1].max() for x, _ in sig.values())
ncap = len(CAPS)
for col, (name, (x, c)) in enumerate(sig.items()):
    ax = fig.add_subplot(4, ncap, 3 * ncap + 1 + col)
    f, t, S = signal.spectrogram(x, fs=FS, window="hann", nperseg=4096, noverlap=3072)
    ax.pcolormesh(t, f / 1000, 10 * np.log10(S + 1e-30),
                  shading="gouraud", vmin=vmax - 90, vmax=vmax, cmap="magma")
    ax.set_ylim(0, FS / 2000); ax.set_xlabel("s"); ax.set_ylabel("kHz")
    ax.set_title(f"STFT - {name}", fontsize=8)

fig.suptitle("MIC4 capture: baseline vs 3 kHz CIC low-pass vs 3 kHz comb notch (narrow/wide) (30 s, max gain)",
             fontsize=13, y=0.997)
fig.subplots_adjust(left=0.06, right=0.98, top=0.96, bottom=0.04, hspace=0.32, wspace=0.22)
fig.savefig(OUT, dpi=110)

# carrier-bin table
print(f"{'tooth':>6} " + " ".join(f"{n:>20}" for n, _, _ in CAPS))
for fc in (3000, 6000, 9000, 12000):
    row = []
    for name, _, _ in CAPS:
        f, p = fine[name]
        row.append(p[np.argmin(np.abs(f - fc))])
    print(f"{fc:>6} " + " ".join(f"{v:>20.1f}" for v in row))
print(f"\nwrote {OUT}")
