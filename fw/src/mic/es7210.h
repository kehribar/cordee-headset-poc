// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef ES7210_H
#define ES7210_H

// ----------------------------------------------------------------------------
#include <stdint.h>

// ----------------------------------------------------------------------------
// All functions return 0 on success, -1 on failure (I2C NACK or bad args).
// ----------------------------------------------------------------------------
#define ES7210_OK   (0)
#define ES7210_FAIL (-1)

// ----------------------------------------------------------------------------
// I2C 7-bit address. Datasheet pattern: 1000 0 AD1 AD0.
// Default board ties AD0=AD1=GND -> 0x40.
// ----------------------------------------------------------------------------
#define ES7210_I2C_ADDR (0x40)

// ----------------------------------------------------------------------------
// Microphone channel identifiers (each maps to MICnP/MICnN pin pair).
// ----------------------------------------------------------------------------
typedef enum
{
  ES7210_MIC1 = 0,
  ES7210_MIC2 = 1,
  ES7210_MIC3 = 2,
  ES7210_MIC4 = 3
} es7210_mic_t;

// ----------------------------------------------------------------------------
// PGA gain codes (reg 0x43..0x46, bits 3:0). Values match datasheet table.
// ----------------------------------------------------------------------------
typedef enum
{
  ES7210_GAIN_0DB    = 0,
  ES7210_GAIN_3DB    = 1,
  ES7210_GAIN_6DB    = 2,
  ES7210_GAIN_9DB    = 3,
  ES7210_GAIN_12DB   = 4,
  ES7210_GAIN_15DB   = 5,
  ES7210_GAIN_18DB   = 6,
  ES7210_GAIN_21DB   = 7,
  ES7210_GAIN_24DB   = 8,
  ES7210_GAIN_27DB   = 9,
  ES7210_GAIN_30DB   = 10,
  ES7210_GAIN_33DB   = 11,
  ES7210_GAIN_34P5DB = 12,
  ES7210_GAIN_36DB   = 13,
  ES7210_GAIN_37P5DB = 14
} es7210_gain_t;

// ----------------------------------------------------------------------------
// Microphone bias voltage codes (reg 0x41/0x42, bits 6:4).
// Recommended VDDM = 3.3V, BIAS = 2.87V (code 7).
// ----------------------------------------------------------------------------
typedef enum
{
  ES7210_BIAS_2P18V = 0,
  ES7210_BIAS_2P26V = 1,
  ES7210_BIAS_2P36V = 2,
  ES7210_BIAS_2P45V = 3,
  ES7210_BIAS_2P55V = 4,
  ES7210_BIAS_2P66V = 5,
  ES7210_BIAS_2P78V = 6,
  ES7210_BIAS_2P87V = 7
} es7210_bias_t;

// ----------------------------------------------------------------------------
// Probe codec via I2C (returns 0 if device ACKs at ES7210_I2C_ADDR).
// ----------------------------------------------------------------------------
int32_t es7210_probe();

// ----------------------------------------------------------------------------
// Read CHIP ID registers (0x3D, 0x3E). Expected: id1=0x72, id0=0x10.
// ----------------------------------------------------------------------------
int32_t es7210_readChipId(uint8_t* id1, uint8_t* id0);

// ----------------------------------------------------------------------------
// One-shot raw register access.
// ----------------------------------------------------------------------------
int32_t es7210_writeReg(uint8_t reg, uint8_t val);
int32_t es7210_readReg(uint8_t reg, uint8_t* val);

// ----------------------------------------------------------------------------
// Default init for I2S slave, 32-bit, single-speed, 48 kHz, MCLK=256*Fs.
// Powers up MIC1+MIC2 -> ADC1/ADC2 (left/right of standard 2-ch I2S frame).
// MIC3/MIC4 stay powered down. PGA gain defaults to 0 dB on all enabled mics.
// Must be called AFTER MCLK is running on the codec MCLK pin.
// ----------------------------------------------------------------------------
int32_t es7210_init();

// ----------------------------------------------------------------------------
// Variant of es7210_init that lets the caller pick which microphones to
// enable. micMask is a 4-bit bitmask, bit n -> ES7210_MICn (n=0..3).
// Selected mics get SELMIC + the supplied gain. Unselected mics have their
// SELMIC bit cleared and gain code 0.
// ----------------------------------------------------------------------------
int32_t es7210_initSel(uint8_t micMask, es7210_gain_t gain);

// ----------------------------------------------------------------------------
// Runtime analog (PGA) gain per microphone.
// ----------------------------------------------------------------------------
int32_t es7210_setMicGain(es7210_mic_t mic, es7210_gain_t gain);

// ----------------------------------------------------------------------------
// Runtime microphone bias voltage per mic-pair (MIC1/2 share, MIC3/4 share).
// pairIndex: 0 -> MIC1/2 (reg 0x41), 1 -> MIC3/4 (reg 0x42).
// ----------------------------------------------------------------------------
int32_t es7210_setMicBias(uint8_t pairIndex, es7210_bias_t bias);

// ----------------------------------------------------------------------------
// Mute / unmute a single mic input by clearing/setting SELMICn (reg 0x43..0x46
// bit 4). PGA gain code is preserved. muted: 0 -> unmuted, !=0 -> muted.
// ----------------------------------------------------------------------------
int32_t es7210_muteMic(es7210_mic_t mic, uint8_t muted);

// ----------------------------------------------------------------------------
#endif
