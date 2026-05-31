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
int32_t es7210_init(void)
{
  // Default: all four mics SELMIC'd at max gain. The actual streaming slot
  // is decided by the PIO capture position.
  return es7210_initSel(0x0F, ES7210_GAIN_37P5DB);
}

int32_t es7210_initSel(uint8_t micMask, es7210_gain_t gain)
{
  int32_t rc = ES7210_OK;

  // ---- Phase 1: configuration (mirrors Espressif es7210_adc_init) ----

  // Soft reset all registers
  rc |= es7210_writeReg(ES7210_REG_RESET, 0xFF);
  sleep_ms(10);

  // Bring the chip out of reset.
  rc |= es7210_writeReg(ES7210_REG_RESET, 0x41);

  // Disable all internal clock paths, then re-enable the ones we need.
  // Espressif starts with 0x3F (everything off) and clears bits per
  // selected mic channel further down.
  rc |= es7210_writeReg(ES7210_REG_CLK_OFF, 0x3F);

  // Initialisation timing.
  rc |= es7210_writeReg(ES7210_REG_TIME_CTRL0, 0x30);
  rc |= es7210_writeReg(ES7210_REG_TIME_CTRL1, 0x30);

  // HPF cutoff defaults (Espressif "quick setup" values).
  rc |= es7210_writeReg(ES7210_REG_ADC12_HPF2, 0x2A);
  rc |= es7210_writeReg(ES7210_REG_ADC12_HPF1, 0x0A);
  rc |= es7210_writeReg(ES7210_REG_ADC34_HPF2, 0x0A);
  rc |= es7210_writeReg(ES7210_REG_ADC34_HPF1, 0x2A);

  // MODE_CFG: bit 0 = master/slave. 0 = slave (we feed BCLK/LRCK).
  rc |= es7210_writeReg(ES7210_REG_MODE_CFG, 0x00);

  // Analog block: PDN_ANA off, VDDA = 3.3 V, VMID 5 kOhm start.
  rc |= es7210_writeReg(ES7210_REG_ANALOG, 0x43);

  // Mic bias: 2.87 V on both pairs (only MIC4 is populated here).
  rc |= es7210_writeReg(ES7210_REG_MIC12_BIAS, 0x70);
  rc |= es7210_writeReg(ES7210_REG_MIC34_BIAS, 0x70);

  // ADC OSR.
  rc |= es7210_writeReg(ES7210_REG_ADC_OSR, 0x20);

  // Main clock: MCLK = 256 * Fs (Fs = 48 kHz, MCLK = 12.288 MHz).
  // adc_div=1 (bits 3:0), doubler=1 (bit 6), dll=1 (bit 7) -> 0xC1.
  rc |= es7210_writeReg(ES7210_REG_MAINCLK, 0xC1);

  // LRCK divider used in master mode only (we run slave).
  rc |= es7210_writeReg(ES7210_REG_MLRCK_DIVH, 0x01);
  rc |= es7210_writeReg(ES7210_REG_MLRCK_DIVL, 0x00);

  // SDP: 32-bit slot width, Left-Justified protocol (no 1-BCLK delay after
  // LRCK transition). Our PIO does not implement the I2S 1-BCLK delay,
  // so LJ aligns the chip's data with our slot capture timing.
  rc |= es7210_writeReg(ES7210_REG_SDP_CFG1, 0x81);

  // SDP: TDM I2S on SDOUT1 (4 channels in one LRCK period).
  rc |= es7210_writeReg(ES7210_REG_SDP_CFG2, 0x02);

  // ALC disabled.
  rc |= es7210_writeReg(ES7210_REG_ALC_SEL, 0x00);

  // ---- Mic select + start (mirrors Espressif's two-phase activation) ----

  // Park both pairs as fully powered down.
  rc |= es7210_writeReg(ES7210_REG_MIC12_PDN, 0xFF);
  rc |= es7210_writeReg(ES7210_REG_MIC34_PDN, 0xFF);

  // Clear SELMIC + gain on every channel first.
  rc |= es7210_writeReg(ES7210_REG_MIC1_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC2_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC3_GAIN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC4_GAIN, 0x00);

  // Re-enable internal clocks for ALL four ADC channels (Espressif clears
  // bits 0x1F from CLK_OFF when MIC1..MIC4 are all selected).
  rc |= es7210_writeReg(ES7210_REG_CLK_OFF, 0x20);

  // Power up BOTH ADC pairs (Espressif writes 0x00 to power up a pair).
  rc |= es7210_writeReg(ES7210_REG_MIC12_PDN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC34_PDN, 0x00);

  // SELMIC + gain per supplied mask.
  for(uint32_t i = 0; i < 4; i++)
  {
    const uint8_t gainCode = (uint8_t)(gain & ES7210_GAIN_CODE_MASK);
    const uint8_t val = (micMask & (1u << i)) ? (ES7210_GAIN_SELMIC | gainCode) : 0x00;
    rc |= es7210_writeReg(kGainRegForMic[i], val);
  }

  // ---- Phase 2: start (mirrors Espressif es7210_start) ----

  rc |= es7210_writeReg(ES7210_REG_CLK_OFF, 0x20);
  rc |= es7210_writeReg(ES7210_REG_PWRDN, 0x00);
  rc |= es7210_writeReg(ES7210_REG_ANALOG, 0x43);

  // MICx_LP: 0x00 = normal mode (datasheet default). Espressif's "0x08"
  // sets bit 3 = LP_PGAn = LOW POWER mode, which silently caps the PGA
  // gain — that was why our PGA gain changes never moved the noise floor.
  rc |= es7210_writeReg(ES7210_REG_MIC1_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC2_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC3_LP, 0x00);
  rc |= es7210_writeReg(ES7210_REG_MIC4_LP, 0x00);

  // Give the auto power-up sequence time to walk to normal state.
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
