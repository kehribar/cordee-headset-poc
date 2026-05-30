// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <stdint.h>
#include "xprintf.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
#include "util.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "pico/util/queue.h"
#include "sound.h"
#include "dac_i2s.h"
#include "mic_i2s.h"
#include "es7210.h"
#include "uart_tx.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "i2c.h"