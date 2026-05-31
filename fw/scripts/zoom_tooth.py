#!/usr/bin/env python3
# Fine-resolution look right at the 3 kHz and 6 kHz comb teeth, all 3 captures.
import wave, numpy as np
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
from scipy import signal

def load(path):
    with wave.open(path,"rb") as w:
        n=w.getnframes(); raw=w.readframes(n)
    b=np.frombuffer(raw,dtype=np.uint8).reshape(-1,3).astype(np.uint32)
    v=(b[:,0]|(b[:,1]<<8)|(b[:,2]<<16)).astype(np.int32)
    v=np.where(v&0x800000,v-(1<<24),v)
    return v.astype(np.float64)/2**23

files=[("baseline","maxgain_30s.wav","tab:red"),
       ("CIC lowpass","cic_3khz.wav","tab:green"),
       ("comb notch a=0.97","notch_3khz.wav","tab:blue")]
fs=48000
# high freq resolution: long segment
nps=1<<17   # 131072 -> ~0.37 Hz/bin
data={}
for name,f,c in files:
    x=load(f); fr,p=signal.welch(x,fs=fs,window="hann",nperseg=nps,noverlap=nps//2)
    data[name]=(fr,10*np.log10(p+1e-30),c)

fig,axs=plt.subplots(1,2,figsize=(15,6))
for ax,fc in zip(axs,(3000,6000)):
    for name,(fr,pdb,c) in data.items():
        m=(fr>=fc-500)&(fr<=fc+500)
        ax.plot(fr[m],pdb[m],lw=1.0,color=c,label=name)
    ax.axvline(fc,color="k",ls=":",lw=0.8)
    ax.axvline(fc-200,color="gray",ls=":",lw=0.6); ax.axvline(fc+200,color="gray",ls=":",lw=0.6)
    ax.set_title(f"{fc} Hz comb tooth (dotted: carrier & +/-200 Hz sidebands)")
    ax.set_xlabel("Hz"); ax.set_ylabel("PSD dB"); ax.grid(alpha=0.3); ax.legend(fontsize=8)
fig.tight_layout(); fig.savefig("tooth_zoom.png",dpi=120)

# exact-bin value at the carrier for each
print(f"{'tooth':>6} " + " ".join(f"{n:>18}" for n,_,_ in files))
for fc in (3000,6000,9000,12000):
    vals=[]
    for name,(fr,pdb,c) in data.items():
        i=np.argmin(np.abs(fr-fc)); vals.append(pdb[i])
    print(f"{fc:>6} " + " ".join(f"{v:>18.1f}" for v in vals))
print("\n(values = PSD dB in the single ~0.37 Hz bin centred on the carrier)")
print("wrote tooth_zoom.png")
