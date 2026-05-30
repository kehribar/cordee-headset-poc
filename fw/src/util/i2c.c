// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "i2c.h"
#include "util.h"
#include "pico/stdlib.h"

// ----------------------------------------------------------------------------
#define i2c_DELAY() sleep_nop(10)

// ----------------------------------------------------------------------------
const uint8_t i2c_sclPin = 5;
const uint8_t i2c_sdaPin = 6;

  // ----------------------------------------------------------------------------
static void i2c_CLOCK_HI()
{
  // Set SCL pin as input
  sio_hw->gpio_oe_clr = (1ul << i2c_sclPin);
}

// ----------------------------------------------------------------------------
static void i2c_DATA_HI()
{
  // Set SDA pin as input
  sio_hw->gpio_oe_clr = (1ul << i2c_sdaPin);
}

// ----------------------------------------------------------------------------
static void i2c_CLOCK_LO()
{
  // Set SCL pin as output
  sio_hw->gpio_oe_set = (1ul << i2c_sclPin);
}

// ----------------------------------------------------------------------------
static void i2c_DATA_LO()
{
  // Set SDA pin as output
  sio_hw->gpio_oe_set = (1ul << i2c_sdaPin);
}

// ----------------------------------------------------------------------------
static void i2c_writeBit(uint8_t c)
{
  if(c > 0)
  {
    i2c_DATA_HI();
  }
  else
  {
    i2c_DATA_LO();
  }

  i2c_DELAY();
  i2c_CLOCK_HI();
  i2c_DELAY();

  i2c_CLOCK_LO();
  i2c_DELAY();

  if(c > 0)
  {
    i2c_DATA_LO();
  }
}

// ----------------------------------------------------------------------------
static uint8_t i2c_readBit()
{
  uint8_t c;

  i2c_DATA_HI();

  i2c_DELAY();
  i2c_CLOCK_HI();
  i2c_DELAY();

  if(gpio_get(i2c_sdaPin) == 0)
  {
    c = 0;
  }
  else
  {
    c = 1;
  }

  i2c_CLOCK_LO();
  i2c_DELAY();

  return c;
}

// ----------------------------------------------------------------------------
static void i2c_writeBit_slow(uint8_t c)
{
  if(c > 0)
  {
    i2c_DATA_HI();
  }
  else
  {
    i2c_DATA_LO();
  }

  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();
  i2c_CLOCK_HI();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  i2c_CLOCK_LO();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  if(c > 0)
  {
    i2c_DATA_LO();
  }
}

// ----------------------------------------------------------------------------
static uint8_t i2c_readBit_slow()
{
  uint8_t c;

  i2c_DATA_HI();

  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();
  i2c_CLOCK_HI();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  if(gpio_get(i2c_sdaPin) == 0)
  {
    c = 0;
  }
  else
  {
    c = 1;
  }

  i2c_CLOCK_LO();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  return c;
}

// ----------------------------------------------------------------------------
void i2c_init()
{
  // I2C pins
  gpio_init(i2c_sdaPin);
  gpio_init(i2c_sclPin);
  gpio_set_dir(i2c_sdaPin, GPIO_OUT);
  gpio_set_dir(i2c_sclPin, GPIO_OUT);

  // ...
  i2c_CLOCK_HI();
  i2c_DATA_HI();

  // ...
  i2c_DELAY();
}

// ----------------------------------------------------------------------------
void i2c_start()
{
  i2c_CLOCK_HI();
  i2c_DATA_HI();
  i2c_DELAY();

  i2c_DATA_LO();
  i2c_DELAY();

  i2c_CLOCK_LO();
  i2c_DELAY();
}

// ----------------------------------------------------------------------------
void i2c_stop()
{
  i2c_DATA_LO();
  i2c_CLOCK_LO();
  i2c_DELAY();

  i2c_CLOCK_HI();
  i2c_DELAY();

  i2c_DATA_HI();
  i2c_DELAY();
}

// ----------------------------------------------------------------------------
void i2c_start_slow()
{
  i2c_CLOCK_HI();
  i2c_DATA_HI();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  i2c_DATA_LO();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  i2c_CLOCK_LO();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();
}

// ----------------------------------------------------------------------------
void i2c_stop_slow()
{
  i2c_DATA_LO();
  i2c_CLOCK_LO();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  i2c_CLOCK_HI();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();

  i2c_DATA_HI();
  i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY(); i2c_DELAY();
}


// ----------------------------------------------------------------------------
uint8_t i2c_write(uint8_t c)
{
  uint8_t i;

  for(i=0;i<8;i++)
  {
    i2c_writeBit(c & 128);

    c <<= 1;
  }

  return i2c_readBit();
}

// ----------------------------------------------------------------------------
uint8_t i2c_read(uint8_t ack)
{
  uint8_t res = 0;
  uint8_t i;

  for (i=0;i<8;i++)
  {
    res <<= 1;
    res |= i2c_readBit();
  }

  if(ack != NO_ACK)
  {
    i2c_writeBit(0);
  }
  else
  {
    i2c_writeBit(1);
  }

  i2c_DELAY();

  return res;
}

// ----------------------------------------------------------------------------
uint8_t i2c_write_slow(uint8_t c)
{
  uint8_t i;

  for(i=0;i<8;i++)
  {
    i2c_writeBit_slow(c & 128);

    c <<= 1;
  }

  return i2c_readBit_slow();
}

// ----------------------------------------------------------------------------
uint8_t i2c_read_slow(uint8_t ack)
{
  uint8_t res = 0;
  uint8_t i;

  for (i=0;i<8;i++)
  {
    res <<= 1;
    res |= i2c_readBit_slow();
  }

  if(ack != NO_ACK)
  {
    i2c_writeBit_slow(0);
  }
  else
  {
    i2c_writeBit_slow(1);
  }

  i2c_DELAY(); i2c_DELAY(); i2c_DELAY();
  i2c_DELAY(); i2c_DELAY();

  return res;
}
