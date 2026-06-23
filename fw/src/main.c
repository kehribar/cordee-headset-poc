// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "main.h"

// ----------------------------------------------------------------------------
uint32_t blink_interval_ms = 1000;

// ----------------------------------------------------------------------------
#define FS (48000)

// ----------------------------------------------------------------------------
static void hardware_init();

// ----------------------------------------------------------------------------
static void i2c_scan()
{
  xprintf("[i2c] scanning ...\r\n");

  uint32_t found = 0;
  for(uint8_t addr=0x00;addr<=0x7F;addr++)
  {
    i2c_start();
    const uint8_t nack = i2c_write(write_address(addr));
    i2c_stop();

    if(nack == ACK)
    {
      xprintf("[i2c] device @ 0x%02X\r\n", addr);
      found++;
    }
  }

  xprintf("[i2c] %u device(s) found\r\n", found);
}

// ----------------------------------------------------------------------------
static void ui_task()
{
  static uint8_t init_flag = false;
  if(init_flag == false)
  {
    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    init_flag = true;
    return;
  }

  // ...
  static uint8_t led_state = 0;
  static uint32_t nextDelay = 0;
  static uint32_t m_lastExecution_ms = 0;
  if(calculateDeltaCounter1ms(m_lastExecution_ms) > nextDelay)
  {
    m_lastExecution_ms = readCounter1ms();

    // ...
    if(led_state == 0) { nextDelay = 50;}
    else if(led_state == 1) { nextDelay = 100; }
    else if(led_state == 2) { nextDelay = 50; }
    else if(led_state == 3) { nextDelay = blink_interval_ms - 200; }

    // ...
    gpio_put(22, (led_state & 0x01) == 0);
    led_state = (led_state + 1) % 4;
  }
}

// ----------------------------------------------------------------------------
// Mic I2S DMA delivered FRAMES_PER_BUFFER MIC4 samples (32-bit left-justified,
// MSB-first). Each PIO transfer captures one channel-4 sample. Forward the
// block to USB unchanged — TinyUSB sends the upper 24 bits on the wire.
// ----------------------------------------------------------------------------
static int32_t s_micPkt[FRAMES_PER_BUFFER];

void __not_in_flash_func(mic_i2s_process)(uint32_t* rx)
{
  for(int32_t i = 0; i < FRAMES_PER_BUFFER; i++)
  {
    s_micPkt[i] = (int32_t)rx[i];
  }
  usb_audio_write_mic(s_micPkt, FRAMES_PER_BUFFER);
}

// ----------------------------------------------------------------------------
// DAC I2S needs FRAMES_PER_BUFFER 32-bit words (L+R interleaved, two words
// per stereo frame). Pull 16-bit mono samples from the USB speaker FIFO,
// duplicate to both channels, left-justify into the 32-bit I2S slot.
// ----------------------------------------------------------------------------
void __not_in_flash_func(dac_i2s_process)(uint32_t* rx, uint32_t* tx)
{
  (void)rx;

  // Startup pre-roll only: wait until ~10 ms of audio has buffered before the
  // first playback so initial USB jitter doesn't immediately underrun. Once
  // started we stay started -- usb_audio_read_spk() zero-fills any momentary
  // shortfall, which is gentler than re-priming the whole buffer (that caused
  // a ~10 ms dropout on every underrun under the unsynced/adaptive clock).
  static bool isWorking = false;
  if(isWorking == false)
  {
    if(usb_audio_spkFifoLevel() < (uint32_t)(FS * 0.010))
    {
      memset(tx, 0, (FRAMES_PER_BUFFER * sizeof(uint32_t)));
      return;
    }
    isWorking = true;
  }

  int16_t pcm[FRAMES_PER_BUFFER / 2];
  usb_audio_read_spk(pcm, FRAMES_PER_BUFFER / 2);

  for(int32_t i = 0, j = 0; i < FRAMES_PER_BUFFER; i += 2, j++)
  {
    const uint32_t w = ((uint32_t)pcm[j]) << 16;
    tx[i + 0] = w;
    tx[i + 1] = w;
  }
}

// ----------------------------------------------------------------------------
int main()
{
  // ...
  hardware_init();

  // ...
  while(1)
  {
    tud_task();
    ui_task();
  }

  // ...
  return 0;
}

// ----------------------------------------------------------------------------
static void hardware_init()
{
  // ...
  set_sys_clock_khz(144000, true);

  // PIO-based UART TX for console output
  uart_tx_init(pio1, 0, 10, 1000000);
  xdev_out(uart_tx_putc);

  // ...
  xprintf("\r\n");
  xprintf("-----------------------\r\n");
  xprintf("Hello World!\r\n\r\n");

  // ...
  const float fs_Hz = FS;
  dac_i2s_init(
       pio0, // PIO ID
          0, // State machine base ID
         26, // I2S peripheral base pin number
      fs_Hz  // Sampling frequency in Hertz
  );

  // Speaker enable (NS4168). dac_i2s_init() above already has the PIO clocking
  // silence (m_txBuf is zero-initialized), so keep the amp shut down until
  // BCLK/LRCK have settled, then bring it up while it is receiving silence.
  // This lets the NS4168 pop-suppression do its job and avoids the class-D
  // power-on pop.
  gpio_init(29);
  gpio_set_dir(29, GPIO_OUT);
  gpio_put(29, false);
  sleep_ms(50);
  gpio_put(29, true);

  // ...
  mic_i2s_init(
       pio1, // PIO ID
          1, // State machine ID
          1, // base pin: SDOUT=1, LRCK=2, BCLK=3
      fs_Hz  // Sampling frequency in Hertz
  );

  // ...
  i2c_init();
  i2c_scan();

  // Only MIC4 is populated. Max analog gain (37.5 dB).
  es7210_initSel(0x08, ES7210_GAIN_37P5DB);

  // ...
  tud_init(BOARD_TUD_RHPORT);
}
