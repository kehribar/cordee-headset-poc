// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "dac_i2s.pio.h"
#include "dac_i2s.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "xprintf.h"

// ----------------------------------------------------------------------------
static uint32_t m_rxBuf[FRAMES_PER_BUFFER * 2];
static uint32_t m_txBuf[FRAMES_PER_BUFFER * 2];

// ----------------------------------------------------------------------------
static uint32_t m_rxBuf_part = 0;
static uint32_t m_txBuf_part = 0;
static uint32_t m_dma_chan_input = 0;
static uint32_t m_dma_chan_output = 0;

// ----------------------------------------------------------------------------
static void dac_i2s_pio_init(
  PIO pio, uint32_t sm, uint32_t base_pin, float fs_Hz
)
{
  // ...
  const uint32_t bclk_pin = base_pin + 0;
  const uint32_t lrck_pin = base_pin + 1;
  const uint32_t dout_pin = base_pin + 2;

  // ...
  const float clkdiv = (float)clock_get_hz(clk_sys) / (fs_Hz * 64 * 8);

  {
    // ...
    pio_gpio_init(pio, dout_pin);
    pio_gpio_init(pio, bclk_pin);
    pio_gpio_init(pio, lrck_pin);

    // ...
    const uint32_t pmask_out = (
      (1u << dout_pin) | (1u << bclk_pin) | (1u << lrck_pin)
    );

    // ...
    pio_sm_set_pindirs_with_mask(pio, sm, pmask_out, pmask_out);
    pio_sm_set_pins_with_mask(pio, sm, pmask_out, pmask_out);

    // ...
    const uint32_t offset = pio_add_program(pio, &dac_i2s_program);
    pio_sm_config sm_config = dac_i2s_program_get_default_config(offset);

    // ...
    sm_config_set_out_pins(&sm_config, dout_pin, 1);
    sm_config_set_sideset_pins(&sm_config, bclk_pin);
    sm_config_set_out_shift(&sm_config, false, true, 32);
    sm_config_set_in_shift(&sm_config, false, true, 32);
    sm_config_set_clkdiv(&sm_config, clkdiv * 2);

    // ...
    pio_sm_init(pio, sm, offset, &sm_config);
    pio_sm_set_enabled(pio, sm, true);
  }
}

// ----------------------------------------------------------------------------
static void __not_in_flash_func(dma_handler)()
{
  if(dma_hw->ints0 & (1u << m_dma_chan_output))
  {
    // Clear IRQ flag
    hw_set_bits(&dma_hw->ints1, (1u << m_dma_chan_output));

    // Swap A/B buffer
    m_txBuf_part ^= 0x01;

    // Initiate another transfer immediately
    uint32_t* const tx_ptr = &(m_txBuf[m_txBuf_part * FRAMES_PER_BUFFER]);
    dma_hw->ch[m_dma_chan_output].al3_read_addr_trig = (uintptr_t)tx_ptr;
  }

  if(dma_hw->ints0 & (1u << m_dma_chan_input))
  {
    // ...
    static uint32_t* rx_ptr_d = m_rxBuf;

    // ...
    const uint32_t m_txBuf_part_next = m_txBuf_part ^ 0x01;
    uint32_t* const tx_ptr = &(m_txBuf[m_txBuf_part_next * FRAMES_PER_BUFFER]);

    // Clear IRQ flag
    hw_set_bits(&dma_hw->ints0, (1u << m_dma_chan_input));

    // Swap A/B buffer
    m_rxBuf_part ^= 0x01;

    // Initiate another transfer immediately
    uint32_t* const rx_ptr = &(m_rxBuf[m_rxBuf_part * FRAMES_PER_BUFFER]);
    dma_hw->ch[m_dma_chan_input].al2_write_addr_trig = (uintptr_t)rx_ptr;

    // Process the buffer
    dac_i2s_process(rx_ptr_d, tx_ptr);

    // ...
    rx_ptr_d = rx_ptr;
  }
}

// ----------------------------------------------------------------------------
static void dac_i2s_dmaout_init(PIO pio, uint32_t sm)
{
  // ...
  m_dma_chan_output = dma_claim_unused_channel(true);
  dma_channel_config cc = dma_channel_get_default_config(m_dma_chan_output);
  xprintf("[dac] m_dma_chan_output: %d\r\n",m_dma_chan_output);

  // ...
  channel_config_set_read_increment(&cc, true);
  channel_config_set_write_increment(&cc, false);
  channel_config_set_dreq(&cc, pio_get_dreq(pio, sm, true));
  channel_config_set_transfer_data_size(&cc, DMA_SIZE_32);

  // ...
  dma_channel_configure(
    m_dma_chan_output, // DMA Channel
    &cc,               // Configuration
    &pio->txf[sm],     // Destination pointer
    m_txBuf,           // Source pointer
    FRAMES_PER_BUFFER, // Number of transfers
    true               // Start immediately
  );

  // ...
  dma_channel_set_irq0_enabled(m_dma_chan_output, true);
}

// ----------------------------------------------------------------------------
static void dac_i2s_dmain_init(PIO pio, uint32_t sm)
{
  // ...
  m_dma_chan_input = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(m_dma_chan_input);
  xprintf("[dac] m_dma_chan_input: %d\r\n",m_dma_chan_input);

  // ...
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c ,true);
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
  dma_channel_set_irq0_enabled(m_dma_chan_input, true);
}

// ----------------------------------------------------------------------------
void dac_i2s_init(PIO pio, uint32_t sm, uint32_t base_pin, float clkdiv)
{
  // ...
  dac_i2s_pio_init(pio, sm, base_pin, clkdiv);
  dac_i2s_dmaout_init(pio, sm);
  dac_i2s_dmain_init(pio, sm);

  // ...
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);
}
