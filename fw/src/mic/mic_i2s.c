// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "mic_i2s.pio.h"
#include "mic_i2s.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "xprintf.h"

// ----------------------------------------------------------------------------
static uint32_t m_rxBuf[FRAMES_PER_BUFFER * 2];

// ----------------------------------------------------------------------------
static uint32_t m_rxBuf_part = 0;
static uint32_t m_dma_chan_input = 0;

// ----------------------------------------------------------------------------
static void mic_i2s_pio_init(
  PIO pio, uint32_t sm, uint32_t base_pin, float fs_Hz
)
{
  // ...
  const uint32_t sdout_pin = base_pin + 0;
  const uint32_t lrck_pin = base_pin + 1;
  const uint32_t bclk_pin = base_pin + 2;

  // 4 PIO cycles per BCLK period; TDM I2S uses BCLK = 128*Fs
  // -> PIO clock = 512 * Fs (was 256 * Fs in 2-ch I2S mode)
  const float clkdiv = (float)clock_get_hz(clk_sys) / (fs_Hz * 128 * 4);

  {
    // ...
    pio_gpio_init(pio, sdout_pin);
    pio_gpio_init(pio, lrck_pin);
    pio_gpio_init(pio, bclk_pin);

    // MITIGATION: the Fs/16 mic comb is LRCK/BCLK digital-edge coupling into
    // the analog front-end. Lowest drive (2 mA) + slow slew on these clock
    // pins cuts the comb carriers by ~5 dB (measured). SDOUT is an input;
    // MCLK is a separate domain proven not to drive the comb, both untouched.
    // See src/mic/notes.md "MIC NOISE INVESTIGATION".
    gpio_set_drive_strength(lrck_pin, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(bclk_pin, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_slew_rate(lrck_pin, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(bclk_pin, GPIO_SLEW_RATE_SLOW);

    // ...
    const uint32_t pmask_out = ((1u << lrck_pin) | (1u << bclk_pin));
    const uint32_t pmask_in  = (1u << sdout_pin);

    // ...
    pio_sm_set_pindirs_with_mask(pio, sm, pmask_out, (pmask_out | pmask_in));
    pio_sm_set_pins_with_mask(pio, sm, 0, pmask_out);

    // ...
    const uint32_t offset = pio_add_program(pio, &mic_i2s_program);
    pio_sm_config sm_config = mic_i2s_program_get_default_config(offset);

    // ...
    sm_config_set_in_pins(&sm_config, sdout_pin);
    sm_config_set_sideset_pins(&sm_config, lrck_pin);
    sm_config_set_in_shift(&sm_config, false, true, 32);
    sm_config_set_clkdiv(&sm_config, clkdiv);

    // ...
    pio_sm_init(pio, sm, offset, &sm_config);
    pio_sm_set_enabled(pio, sm, true);
  }
}

// ----------------------------------------------------------------------------
static void __not_in_flash_func(dma_handler)()
{
  if(dma_hw->ints1 & (1u << m_dma_chan_input))
  {
    // ...
    static uint32_t* rx_ptr_d = m_rxBuf;

    // Clear IRQ flag
    hw_set_bits(&dma_hw->ints1, (1u << m_dma_chan_input));

    // Swap A/B buffer
    m_rxBuf_part ^= 0x01;

    // Initiate another transfer immediately
    uint32_t* const rx_ptr = &(m_rxBuf[m_rxBuf_part * FRAMES_PER_BUFFER]);
    dma_hw->ch[m_dma_chan_input].al2_write_addr_trig = (uintptr_t)rx_ptr;

    // Process the just-completed buffer
    mic_i2s_process(rx_ptr_d);

    // ...
    rx_ptr_d = rx_ptr;
  }
}

// ----------------------------------------------------------------------------
static void mic_i2s_dmain_init(PIO pio, uint32_t sm)
{
  // ...
  m_dma_chan_input = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(m_dma_chan_input);
  xprintf("[mic] m_dma_chan_input: %d\r\n", m_dma_chan_input);

  // ...
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);

  // ...
  dma_channel_configure(
    m_dma_chan_input,  // DMA Channel
    &c,                // Configuration
    m_rxBuf,           // Destination pointer
    &pio->rxf[sm],     // Source pointer
    FRAMES_PER_BUFFER, // Number of transfers
    true               // Start immediately
  );

  // ...
  dma_channel_set_irq1_enabled(m_dma_chan_input, true);
}

// ----------------------------------------------------------------------------
void mic_mclk_init(PIO pio, uint32_t sm, uint32_t pin, float fs_Hz)
{
  // Standard 256*fs MCLK; ES7210 PLL auto-locks
  const float mclk_hz = fs_Hz * 256.0f;

  // 2 PIO cycles per MCLK period
  const float clkdiv = (float)clock_get_hz(clk_sys) / (mclk_hz * 2.0f);

  // ...
  pio_gpio_init(pio, pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

  // ...
  const uint32_t offset = pio_add_program(pio, &mic_mclk_program);
  pio_sm_config sm_config = mic_mclk_program_get_default_config(offset);

  // ...
  sm_config_set_set_pins(&sm_config, pin, 1);
  sm_config_set_clkdiv(&sm_config, clkdiv);

  // ...
  pio_sm_init(pio, sm, offset, &sm_config);
  pio_sm_set_enabled(pio, sm, true);
}

// ----------------------------------------------------------------------------
void mic_i2s_init(PIO pio, uint32_t sm, uint32_t base_pin, float fs_Hz)
{
  // MCLK must be running before the ES7210 will ACK on I2C
  mic_mclk_init(pio, sm + 1, base_pin + 3, fs_Hz);

  // ...
  mic_i2s_pio_init(pio, sm, base_pin, fs_Hz);
  mic_i2s_dmain_init(pio, sm);

  // ...
  irq_set_exclusive_handler(DMA_IRQ_1, dma_handler);
  irq_set_enabled(DMA_IRQ_1, true);
}
