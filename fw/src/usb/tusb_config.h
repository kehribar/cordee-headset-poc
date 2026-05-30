// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif

// ----------------------------------------------------------------------------
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#define CFG_TUD_ENABLED   1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// ----------------------------------------------------------------------------
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

#define CFG_TUD_CDC    0
#define CFG_TUD_MSC    0
#define CFG_TUD_HID    0
#define CFG_TUD_MIDI   0
#define CFG_TUD_AUDIO  1
#define CFG_TUD_VENDOR 0

// ----------------------------------------------------------------------------
// 1ch speaker (16-bit) + 1ch mic (24-bit) @ 48 kHz full-speed
//
// FS packet size: ((48000 + 999) / 1000 + 1) = 49 samples max per frame
//   Speaker EP: 49 * 2 bytes * 1ch = 98 bytes
//   Mic EP:     49 * 3 bytes * 1ch = 147 bytes
//
// Total audio function descriptor length:
//   IAD(8) + StdAC(9) + CSAC(9)
//   + ACentities = CLK(8) + IT(17) + OT(12) + IT(17) + OT(12) = 66
//   + SPK AS: alt0(9) + alt1(9) + CS_AS_INT(16) + TypeI(6)
//            + StdISO_EP(7) + CS_ISO_EP(8) + StdISO_FB_EP(7) = 62
//   + MIC AS: alt0(9) + alt1(9) + CS_AS_INT(16) + TypeI(6)
//            + StdISO_EP(7) + CS_ISO_EP(8) = 55
//   = 26 + 66 + 62 + 55 = 209
// ----------------------------------------------------------------------------
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN        202
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT        2
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ     64

#define CFG_TUD_AUDIO_ENABLE_EP_OUT               1
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX        98
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ     (98 * 8)

#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP                  0

#define CFG_TUD_AUDIO_ENABLE_EP_IN                1
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX         147
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ      (147 * 8)

#define CFG_TUD_AUDIO_EP_IN_FLOW_CONTROL          0

#ifdef __cplusplus
}
#endif

#endif
