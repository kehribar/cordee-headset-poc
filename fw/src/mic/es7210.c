// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "es7210.h"
#include "i2c.h"
#include "util.h"
#include "xprintf.h"

// ----------------------------------------------------------------------------
// Register map (subset used by this driver). Names track datasheet.
// ----------------------------------------------------------------------------
#define ES7210_REG_RESET             (0x00)
#define ES7210_REG_CLK_OFF           (0x01)
#define ES7210_REG_MAINCLK           (0x02)
#define ES7210_REG_MASTERCLK         (0x03)
#define ES7210_REG_MLRCK_DIVH        (0x04)
#define ES7210_REG_MLRCK_DIVL        (0x05)
#define ES7210_REG_PWRDN             (0x06)
#define ES7210_REG_ADC_OSR           (0x07)
#define ES7210_REG_MODE_CFG          (0x08)
#define ES7210_REG_TIME_CTRL0        (0x09)
#define ES7210_REG_TIME_CTRL1        (0x0A)
#define ES7210_REG_CHIP_STATUS       (0x0B)
#define ES7210_REG_INT_CTRL          (0x0C)
#define ES7210_REG_MISC              (0x0D)
#define ES7210_REG_DMIC_CTRL         (0x10)
#define ES7210_REG_SDP_CFG1          (0x11)
#define ES7210_REG_SDP_CFG2          (0x12)
#define ES7210_REG_ADC_CTRL          (0x13)
#define ES7210_REG_ADC34_CTRL        (0x14)
#define ES7210_REG_ADC12_CTRL        (0x15)
#define ES7210_REG_ALC_SEL           (0x16)
#define ES7210_REG_ALC_CFG1          (0x17)
#define ES7210_REG_ADC34_HPF1        (0x21)
#define ES7210_REG_ADC34_HPF2        (0x20)
#define ES7210_REG_ADC12_HPF1        (0x22)
#define ES7210_REG_ADC12_HPF2        (0x23)
#define ES7210_REG_CHIP_ID1          (0x3D)
#define ES7210_REG_CHIP_ID0          (0x3E)
#define ES7210_REG_ANALOG            (0x40)
#define ES7210_REG_MIC12_BIAS        (0x41)
#define ES7210_REG_MIC34_BIAS        (0x42)
#define ES7210_REG_MIC1_GAIN         (0x43)
#define ES7210_REG_MIC2_GAIN         (0x44)
#define ES7210_REG_MIC3_GAIN         (0x45)
#define ES7210_REG_MIC4_GAIN         (0x46)
#define ES7210_REG_MIC1_LP           (0x47)
#define ES7210_REG_MIC2_LP           (0x48)
#define ES7210_REG_MIC3_LP           (0x49)
#define ES7210_REG_MIC4_LP           (0x4A)
#define ES7210_REG_MIC12_PDN         (0x4B)
#define ES7210_REG_MIC34_PDN         (0x4C)

// ----------------------------------------------------------------------------
// MICn_GAIN register layout (bit4 = SELMICn, bits 3:0 = gain code).
// ----------------------------------------------------------------------------
#define ES7210_GAIN_SELMIC           (1u << 4)
#define ES7210_GAIN_CODE_MASK        (0x0F)

// ----------------------------------------------------------------------------
static const uint8_t kGainRegForMic[4] =
{
  ES7210_REG_MIC1_GAIN,
  ES7210_REG_MIC2_GAIN,
  ES7210_REG_MIC3_GAIN,
  ES7210_REG_MIC4_GAIN
};

// ----------------------------------------------------------------------------
int32_t es7210_writeReg(uint8_t reg, uint8_t val)
{
  i2c_start();
  const uint8_t a1 = i2c_write(write_address(ES7210_I2C_ADDR));
  const uint8_t a2 = i2c_write(reg);
  const uint8_t a3 = i2c_write(val);
  i2c_stop();

  if((a1 != ACK) || (a2 != ACK) || (a3 != ACK))
  {
    return ES7210_FAIL;
  }

  return ES7210_OK;
}

// ----------------------------------------------------------------------------
int32_t es7210_readReg(uint8_t reg, uint8_t* val)
{
  // Phase 1: write register pointer
  i2c_start();
  const uint8_t a1 = i2c_write(write_address(ES7210_I2C_ADDR));
  const uint8_t a2 = i2c_write(reg);

  // Phase 2: repeated start + read
  i2c_start();
  const uint8_t a3 = i2c_write(read_address(ES7210_I2C_ADDR));
  *val = i2c_read(NO_ACK);
  i2c_stop();

  if((a1 != ACK) || (a2 != ACK) || (a3 != ACK))
  {
    return ES7210_FAIL;
  }

  return ES7210_OK;
}

// ----------------------------------------------------------------------------
int32_t es7210_probe()
{
  i2c_start();
  const uint8_t nack = i2c_write(write_address(ES7210_I2C_ADDR));
  i2c_stop();

  if(nack != ACK)
  {
    return ES7210_FAIL;
  }

  return ES7210_OK;
}

// ----------------------------------------------------------------------------
int32_t es7210_readChipId(uint8_t* id1, uint8_t* id0)
{
  const int32_t r1 = es7210_readReg(ES7210_REG_CHIP_ID1, id1);
  const int32_t r0 = es7210_readReg(ES7210_REG_CHIP_ID0, id0);

  if((r1 != ES7210_OK) || (r0 != ES7210_OK))
  {
    return ES7210_FAIL;
  }

  return ES7210_OK;
}

// ----------------------------------------------------------------------------
int32_t es7210_init()
{
  int32_t rc = ES7210_OK;

  // Soft reset all registers
  rc |= es7210_writeReg(ES7210_REG_RESET, 0xFF);
  sleep_ms(10);

  // Exit reset: CSM_ON=1, SEQ_DIS=0 (auto power sequence enabled)
  rc |= es7210_writeReg(ES7210_REG_RESET, 0x32);

  // Enable all clocks (turn off the master-mode SCLK/LRCK drivers since we
  // are I2S slave, but keep MCLK + ADC clocks running).
  // bit5 MASTER_CLK_OFF=1, bit6 EXT_SCLKLRCK_OFF=0 (slave SCLK/LRCK enabled)
  rc |= es7210_writeReg(ES7210_REG_CLK_OFF, 0x20);

  // Main clock control: ADC clock multiplier x2, ADC divider /1
  // Suited to MCLK = 256*Fs configuration.
  rc |= es7210_writeReg(ES7210_REG_MAINCLK, 0xC1);

  // Master clock control: MCLK from pad, SCLK divide /4 (unused in slave)
  rc |= es7210_writeReg(ES7210_REG_MASTERCLK, 0x04);

  // Master LRCK divider (unused in slave mode, leave as default 256)
  rc |= es7210_writeReg(ES7210_REG_MLRCK_DIVH, 0x01);
  rc |= es7210_writeReg(ES7210_REG_MLRCK_DIVL, 0x00);

  // Power: DLL on, internal pull-ups on, TDMIN pulldown on
  rc |= es7210_writeReg(ES7210_REG_PWRDN, 0x00);

  // ADC OSR (default 0x20 = 32)
  rc |= es7210_writeReg(ES7210_REG_ADC_OSR, 0x20);

  // Mode: SCLK normal, EQ off, single-speed (Fs<=48kHz), I2S slave.
  // LRCK_RATE_MODE = 2 (4 channels, used by some TDM variants).
  rc |= es7210_writeReg(ES7210_REG_MODE_CFG, 0x24);

  // Initialization timing
  rc |= es7210_writeReg(ES7210_REG_TIME_CTRL0, 0x30);
  rc |= es7210_writeReg(ES7210_REG_TIME_CTRL1, 0x30);

  // Misc control (default DELAY_SEL=01 / 5ns)
  rc |= es7210_writeReg(ES7210_REG_MISC, 0x01);

  // DMIC off (analog mic mode)
  rc |= es7210_writeReg(ES7210_REG_DMIC_CTRL, 0x00);

  // SDP: 32-bit word length (SP_WL=100), I2S protocol (SP_PROTOCAL=00)
  rc |= es7210_writeReg(ES7210_REG_SDP_CFG1, 0x80);

  // SDP: TDM I2S on SDOUT1 (SDOUT_MODE=10).
  // Frame: ch1, ch3 during LRCK=H, ch2, ch4 during LRCK=L (Figure 2e).
  rc |= es7210_writeReg(ES7210_REG_SDP_CFG2, 0x02);

  // ALC disabled, plain PGA gain mode
  rc |= es7210_writeReg(ES7210_REG_ALC_SEL, 0x00);

  // HPF defaults (DC blocker on, slow setting)
  rc |= es7210_writeReg(ES7210_REG_ADC12_HPF1, 0x06);
  rc |= es7210_writeReg(ES7210_REG_ADC12_HPF2, 0x26);
  rc |= es7210_writeReg(ES7210_REG_ADC34_HPF1, 0x26);
  rc |= es7210_writeReg(ES7210_REG_ADC34_HPF2, 0x06);

  // Analog: PDN_ANA off (bit7=0), VX2OFF=1 for VDDA=3.3V (bit6=1)
  rc |= es7210_writeReg(ES7210_REG_ANALOG, 0x43);

  // Mic bias: 2.87V on MIC3/4 pair (only MIC4 used). MIC1/2 pair off.
  rc |= es7210_writeReg(ES7210_REG_MIC12_BIAS, 0x70);
  rc |= es7210_writeReg(ES7210_REG_MIC34_BIAS, 0x70);

  // Only MIC4 selected at 0 dB; MIC1/2/3 deselected.
  rc |= es7210_writeReg(ES7210_REG_MIC1_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC2_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC3_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC4_GAIN, ES7210_GAIN_SELMIC | ES7210_GAIN_33DB);

  // Low-power bits off on all mics
  rc |= es7210_writeReg(ES7210_REG_MIC1_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC2_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC3_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC4_LP, 0x00);

  // Power down ADC1/ADC2 entirely; MIC1/2 pair off.
  rc |= es7210_writeReg(ES7210_REG_MIC12_PDN, 0xFF);

  // ADC34 pair: ref-gen + MICBIAS34 + ADC4 + PGA4 ON; ADC3/PGA3 OFF.
  // bits: PDN_ADC34VREFGEN=0, PDN_MICBIAS34=0, PDN_PGA4=0, PDN_PGA3=1,
  //       PDN_MOD4=0, PDN_MOD3=1, MODTOP4_RST=0, MODTOP3_RST=1  -> 0x15
  rc |= es7210_writeReg(ES7210_REG_MIC34_PDN, 0x15);

  // Release all digital resets and turn the chip state machine ON.
  // Without this, ADC modulators stay off and SDOUT1 outputs digital zero.
  // bits: RST_MSTGEN=0, RST_ADC34_DIG=0, RST_ADC12_DIG=0, SEQ_DIS=0,
  //       RST_REGS=0, RST_DIG=0, CSM_ON=1  -> 0x01
  rc |= es7210_writeReg(ES7210_REG_RESET, 0x01);

  // Give the auto power-up sequence time to walk through chip-initial -> normal
  sleep_ms(50);

  // Probe chip ID for sanity
  uint8_t id1 = 0;
  uint8_t id0 = 0;
  if(es7210_readChipId(&id1, &id0) == ES7210_OK)
  {
    xprintf("[es7210] chip id: %02X%02X\r\n", id1, id0);
  }
  else
  {
    xprintf("[es7210] chip id read FAILED\r\n");
    rc |= ES7210_FAIL;
  }

  if(rc != ES7210_OK)
  {
    return ES7210_FAIL;
  }

  return ES7210_OK;
}

// ----------------------------------------------------------------------------
int32_t es7210_setMicGain(es7210_mic_t mic, es7210_gain_t gain)
{
  if((uint32_t)mic > 3)
  {
    return ES7210_FAIL;
  }

  const uint8_t reg = kGainRegForMic[(uint32_t)mic];

  uint8_t cur = 0;
  if(es7210_readReg(reg, &cur) != ES7210_OK)
  {
    return ES7210_FAIL;
  }

  // Preserve SELMICn bit, replace gain code
  const uint8_t next = (
    (cur & ES7210_GAIN_SELMIC) | ((uint8_t)gain & ES7210_GAIN_CODE_MASK)
  );

  return es7210_writeReg(reg, next);
}

// ----------------------------------------------------------------------------
int32_t es7210_setMicBias(uint8_t pairIndex, es7210_bias_t bias)
{
  const uint8_t reg = (
    ((pairIndex == 0) ? ES7210_REG_MIC12_BIAS : ES7210_REG_MIC34_BIAS)
  );

  uint8_t cur = 0;
  if(es7210_readReg(reg, &cur) != ES7210_OK)
  {
    return ES7210_FAIL;
  }

  // bits 6:4 = LVL_MICBIAS
  const uint8_t next = (cur & ~(0x70)) | (((uint8_t)bias & 0x07) << 4);

  return es7210_writeReg(reg, next);
}

// ----------------------------------------------------------------------------
int32_t es7210_muteMic(es7210_mic_t mic, uint8_t muted)
{
  if((uint32_t)mic > 3)
  {
    return ES7210_FAIL;
  }

  const uint8_t reg = kGainRegForMic[(uint32_t)mic];

  uint8_t cur = 0;
  if(es7210_readReg(reg, &cur) != ES7210_OK)
  {
    return ES7210_FAIL;
  }

  // muted -> clear SELMICn; unmuted -> set it
  const uint8_t next = (
    ((muted != 0) ? (cur & ~ES7210_GAIN_SELMIC) : (cur | ES7210_GAIN_SELMIC))
  );

  return es7210_writeReg(reg, next);
}
