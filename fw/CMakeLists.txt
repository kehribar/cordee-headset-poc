# -----------------------------------------------------------------------------
#
#
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)

# ...
set(PROJECT_NAME usb_inout)
set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350)

# ...
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ...
include(./scripts/pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)

# ...
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ...
pico_sdk_init()

# ...
add_executable(${PROJECT_NAME})

# ...
pico_generate_pio_header(${PROJECT_NAME}
  ../src/dac/dac_i2s.pio OUTPUT_DIR src/dac
)

# ...
pico_generate_pio_header(${PROJECT_NAME}
  ../src/mic/mic_i2s.pio OUTPUT_DIR src/mic
)

# ...
pico_generate_pio_header(${PROJECT_NAME}
  ../src/util/uart_tx.pio OUTPUT_DIR src/util
)

# ...
target_include_directories(${PROJECT_NAME} PRIVATE
  src
  src/dac
  src/mic
  src/com
  src/util
  src/usb
)

# ...
target_sources(${PROJECT_NAME} PRIVATE

  # ...
  src/main.c

  # ...
  src/util/xprintf.c
  src/util/i2c.c
  src/util/uart_tx.c

  # ...
  src/dac/dac_i2s.c

  # ...
  src/mic/mic_i2s.c
  src/mic/es7210.c
)

# ...
target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_bootrom
  pico_stdlib
  hardware_pio
  hardware_dma
  hardware_irq
  hardware_pwm
)

# ...
pico_add_extra_outputs(${PROJECT_NAME})
