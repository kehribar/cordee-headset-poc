#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Quick WAV "fingerprint" used during the TDM-decode investigation.
#
# Prints: sample count, rate, non-zero fraction, which phases (mod 16) carry
# data, RMS/peak, and the dominant spectral bins. The non-zero fraction +
# hot-slot list is what exposed the broken capture: a healthy mic stream is
# nz~1.0 with no hot-slot structure; the bug shows nz=0.125 (1/8) with data
# locked to 2 of every 16 samples (the Fs/16 zero-stuffing comb).
#   usage: fp.py file.wav
# -----------------------------------------------------------------------------
import numpy as np, wave, sys


def load(fn):
    w = wave.open(fn, 'rb')
    sw, ch, fr, n = w.getsampwidth(), w.getnchannels(), w.getframerate(), w.getnframes()
    raw = w.readframes(n)
    if sw == 3:                                   # 24-bit packed LE -> int32
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        x = (b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16))
        x = np.where(x & 0x800000, x - (1 << 24), x)
    elif sw == 2:
        x = np.frombuffer(raw, dtype='<i2').astype(np.int32)
    else:
        x = np.frombuffer(raw, dtype='<i4')
    if ch > 1:
        x = x[::ch]
    return x, fr


def main():
    fn = sys.argv[1]
    x, fr = load(fn)
    nz = np.count_nonzero(x) / len(x)
    m = x[:len(x) // 16 * 16].reshape(-1, 16)
    prof = (m != 0).mean(0)
    hot = [i for i, p in enumerate(prof) if p > 0.5]
    print(f"{fn}: n={len(x)} rate={fr} nz={nz:.4f} hot16={hot}")
    print("  rms=%.1f  max=%d" % (np.sqrt(np.mean(x.astype(float) ** 2)), np.abs(x).max()))
    xf = x.astype(float) - x.mean()
    f = np.fft.rfftfreq(len(xf), 1 / fr)
    S = np.abs(np.fft.rfft(xf))
    top = np.argsort(S)[-5:][::-1]
    print("  top freqs(Hz):", [round(float(f[i]), 1) for i in top])


if __name__ == "__main__":
    main()
