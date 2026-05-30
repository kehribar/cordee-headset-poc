// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef UTIL_H
#define UTIL_H

// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/time.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

// ----------------------------------------------------------------------------
#define FRAMES_PER_BUFFER (96 * 1)

// ----------------------------------------------------------------------------
static uint32_t readCounter1ms()
{
  return to_ms_since_boot(get_absolute_time());
}

// ----------------------------------------------------------------------------
static uint32_t calculateDeltaCounter1ms(uint32_t ref)
{
  uint32_t now = readCounter1ms();
  if(now < ref)
  {
    return 0;
  }
  return now - ref;
}

// ----------------------------------------------------------------------------
static void sleep_nop(const uint32_t nopAmout)
{
  uint32_t i = nopAmout;

  while(i--)
  {
    __asm("nop");
  }
}

// ----------------------------------------------------------------------------
#endif
