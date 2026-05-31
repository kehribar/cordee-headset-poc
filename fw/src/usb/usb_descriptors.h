// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include "tusb.h"

// ----------------------------------------------------------------------------
// Entity IDs within the audio control block
// ----------------------------------------------------------------------------
#define AUDIO_CLK_ID     0x01
#define AUDIO_SPK_IT_ID  0x02
#define AUDIO_SPK_OT_ID  0x03
#define AUDIO_MIC_IT_ID  0x04
#define AUDIO_MIC_OT_ID  0x05

// ----------------------------------------------------------------------------
// Interface numbers
// ----------------------------------------------------------------------------
enum {
  ITF_NUM_AUDIO_CONTROL = 0,
  ITF_NUM_AUDIO_SPK,
  ITF_NUM_AUDIO_MIC,
  ITF_NUM_TOTAL
};

// ----------------------------------------------------------------------------
// Endpoint numbers (RP2350 supports EP1 IN and EP1 OUT simultaneously)
// ----------------------------------------------------------------------------
#define EPNUM_AUDIO_SPK     0x01
#define EPNUM_AUDIO_MIC     0x01
#define EPNUM_AUDIO_SPK_FB  0x02

// ----------------------------------------------------------------------------
// Audio format parameters
// ----------------------------------------------------------------------------
#define AUDIO_SAMPLE_RATE  48000   // keep == FS in main.c
#define AUDIO_SPK_CH       1
#define AUDIO_SPK_BYTES    2
#define AUDIO_SPK_BITS     16
#define AUDIO_MIC_CH       1
#define AUDIO_MIC_BYTES    3
#define AUDIO_MIC_BITS     24

// Full-speed (1 ms frame) max samples/frame, +1 for async clock drift.
//   48k -> 49,  60k -> 61
#define AUDIO_MAX_SPF      (((AUDIO_SAMPLE_RATE) + 999) / 1000 + 1)
#define AUDIO_EP_SPK_SZ    (AUDIO_MAX_SPF * AUDIO_SPK_BYTES * AUDIO_SPK_CH)  // 60k:122
#define AUDIO_EP_MIC_SZ    (AUDIO_MAX_SPF * AUDIO_MIC_BYTES * AUDIO_MIC_CH)  // 60k:183

// ----------------------------------------------------------------------------
// AC class-specific block length (entities only, excludes CS_AC header)
// ----------------------------------------------------------------------------
#define AUDIO_AC_ENTITIES_LEN ( \
  TUD_AUDIO_DESC_CLK_SRC_LEN + \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN \
)

// Full audio function descriptor length (must match CFG_TUD_AUDIO_FUNC_1_DESC_LEN).
// Speaker EP runs in ASYNC mode without an explicit feedback EP — the host
// estimates rate from the data stream. Adding a feedback EP confuses the
// Linux UAC2 driver here, blocking the mic clock probe; see commit log.
#define AUDIO_FUNC_DESC_LEN ( \
  TUD_AUDIO_DESC_IAD_LEN + \
  TUD_AUDIO_DESC_STD_AC_LEN + \
  TUD_AUDIO_DESC_CS_AC_LEN + \
  AUDIO_AC_ENTITIES_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_CS_AS_INT_LEN + \
  TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + \
  TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + \
  TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_CS_AS_INT_LEN + \
  TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + \
  TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + \
  TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN \
)

#endif
