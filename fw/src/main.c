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
#define SPK_DATA_LEN (2048)
static uint32_t spkData[SPK_DATA_LEN];
volatile uint32_t spkDataHead = 0;
volatile uint32_t spkDataTail = 0;

// ----------------------------------------------------------------------------
static inline bool spkFifoIsFull()
{
  return ((spkDataHead + 1) & (SPK_DATA_LEN - 1)) == spkDataTail;
}

// ----------------------------------------------------------------------------
static inline bool spkFifoIsEmpty()
{
  return spkDataHead == spkDataTail;
}

// ----------------------------------------------------------------------------
static inline uint32_t spkFifoLevel()
{
  return (spkDataHead - spkDataTail) & (SPK_DATA_LEN - 1);
}

// ----------------------------------------------------------------------------
static inline uint32_t spkFifoAvailable()
{
  return (SPK_DATA_LEN - 1) - spkFifoLevel();
}

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
void __not_in_flash_func(mic_i2s_process)(uint32_t* rx)
{

}

// ----------------------------------------------------------------------------
void __not_in_flash_func(dac_i2s_process)(uint32_t* rx, uint32_t* tx)
{
  bool silence = false;
  static bool isWorking = false;
  const uint32_t level = spkFifoLevel();
  if(isWorking)
  {
    if(level < FRAMES_PER_BUFFER)
    {
      silence = true;
    }
  }
  else
  {
    // Wait for initial 10ms data
    if(level < (int32_t)(FS * 0.010))
    {
      silence = true;
    }
  }

  if(silence)
  {
    isWorking = false;
    memset(tx, 0, (FRAMES_PER_BUFFER * sizeof(uint32_t)));
  }
  else
  {
    isWorking = true;
    int32_t tail = spkDataTail;
    for(int32_t i=0;i<FRAMES_PER_BUFFER;i+=2)
    {
      int32_t data = spkData[tail++];
      tail &= (SPK_DATA_LEN - 1);

      tx[i + 0] = ((int32_t)data << 16);
      tx[i + 1] = ((int32_t)data << 16);
    }
    spkDataTail = tail;
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

  // Speaker enable
  gpio_init(29);
  gpio_set_dir(29, GPIO_OUT);
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

  // ...
  es7210_init();
  es7210_setMicGain(ES7210_MIC4, ES7210_GAIN_37P5DB);
}
