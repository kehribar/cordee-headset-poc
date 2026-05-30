// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "uart_tx.pio.h"
#include "uart_tx.h"
#include "hardware/clocks.h"

// ----------------------------------------------------------------------------
static PIO m_pio;
static uint32_t m_sm;

// ----------------------------------------------------------------------------
void uart_tx_init(PIO pio, uint32_t sm, uint32_t tx_pin, uint32_t baud_rate)
{
  m_pio = pio;
  m_sm = sm;

  // ...
  const uint32_t offset = pio_add_program(pio, &uart_tx_program);
  pio_sm_config c = uart_tx_program_get_default_config(offset);

  // ...
  sm_config_set_out_pins(&c, tx_pin, 1);
  sm_config_set_sideset_pins(&c, tx_pin);
  sm_config_set_out_shift(&c, true, false, 32);

  // ...
  const float clkdiv = (float)clock_get_hz(clk_sys) / (float)(baud_rate * 8);
  sm_config_set_clkdiv(&c, clkdiv);

  // ...
  pio_gpio_init(pio, tx_pin);
  pio_sm_set_pins_with_mask(pio, sm, (1u << tx_pin), (1u << tx_pin));
  pio_sm_set_pindirs_with_mask(pio, sm, (1u << tx_pin), (1u << tx_pin));

  // ...
  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_enabled(pio, sm, true);
}

// ----------------------------------------------------------------------------
void uart_tx_putc(uint8_t c)
{
  pio_sm_put_blocking(m_pio, m_sm, (uint32_t)c);
}
