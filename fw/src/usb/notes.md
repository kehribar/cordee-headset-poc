# USB audio notes

Device: composite UAC2, VID/PID `cafe:4011`, IAD + 3 interfaces (AudioControl,
AS-speaker OUT 1ch/16-bit, AS-mic IN 1ch/24-bit), all @ 48 kHz full-speed.

# Windows "Device not started (usbaudio2)" — FIXED 2026-05-31

## Symptom
On Windows the device matched the in-box `usbaudio2.inf` but the Events tab
showed "Device configured" then **"Device not started (usbaudio2)"** /
"requires further installation". The *whole* audio function failed, so neither
mic nor speaker worked. Linux (Pi `arecord`) and macOS were always fine.

## Cause
The speaker OUT isochronous endpoint was declared **ASYNC** but had **no
isochronous feedback IN endpoint** (`CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP 0`).
UAC2 §3.16.2.2 requires a feedback EP for an asynchronous sink. Windows'
`usbaudio2.sys` enforces this strictly and refuses to start the function;
Linux/macOS tolerate its absence.

## Fix
Speaker endpoint sync type **ASYNC → ADAPTIVE** in `usb_descriptors.c`
(`TUSB_ISO_EP_ATT_ADAPTIVE`). Adaptive sinks need no feedback EP, so Windows
starts AND Linux is unaffected (no feedback EP to confuse its clock probe — a
feedback EP had previously broken the Linux mic clock probe, which is why we
didn't just add one). One descriptor bit; no length change.

Status: flashed. Re-confirm on Windows that Events now says "Device started" and
the Cordee Headset shows up as both record + playback. If Windows still refuses,
the spec-correct fallback is a proper feedback EP (more work, must not re-break
Linux).

## Clock control
`tud_audio_get_req_entity_cb` answers GET CUR / RANGE / CLK_VALID for the clock
source (fixed 48 kHz). That part is fine — not a cause of the Windows failure.
