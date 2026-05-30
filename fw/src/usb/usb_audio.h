// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"
#include "usb_descriptors.h"
#include "hardware/structs/usb.h"

// ----------------------------------------------------------------------------
// Push n_frames of mono mic samples to the USB IN endpoint. Samples are
// sign-extended in the upper bits of int32_t (Q31 / left-justified). Only
// the upper 24 bits are transmitted on the wire.
// Returns false if the host hasn't subscribed (alt 0) or isn't enumerated.
// ----------------------------------------------------------------------------
bool usb_audio_write_mic(const int32_t* mic_samples, int32_t n_frames);

// ----------------------------------------------------------------------------
// Pop up to n_frames mono 16-bit speaker samples (sign-extended to int16
// stored in upper 16 bits of int32_t). Missing samples → zero.
// ----------------------------------------------------------------------------
int32_t usb_audio_read_spk(int16_t* out, int32_t n_frames);

// ----------------------------------------------------------------------------
uint32_t usb_audio_spkFifoLevel(void);

// ----------------------------------------------------------------------------
#endif
