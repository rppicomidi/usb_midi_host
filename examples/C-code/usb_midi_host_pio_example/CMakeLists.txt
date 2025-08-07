cmake_minimum_required(VERSION 3.13)

set(BOARD pico_sdk)
include(pico_sdk_import.cmake)

project(usb_midi_host_pio_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
add_executable(usb_midi_host_pio_example
    usb_midi_host_pio_example.c
)

pico_enable_stdio_uart(usb_midi_host_pio_example 1)
#pico_enable_stdio_usb(usb_midi_host_pio_example 1)

target_include_directories(usb_midi_host_pio_example PRIVATE
 ${CMAKE_CURRENT_LIST_DIR}
)

target_link_options(usb_midi_host_pio_example PRIVATE -Xlinker --print-memory-usage)
target_compile_options(usb_midi_host_pio_example PRIVATE -Wall -Wextra)
target_link_libraries(usb_midi_host_pio_example tinyusb_host tinyusb_board
  pico_multicore tinyusb_pico_pio_usb pico_stdlib)

if(DEFINED PICO_BOARD)
if(PICO_BOARD MATCHES pico_w)
message("Adding pico_cyw43_arch_none library for pico_w")
# add additional compile and link options
target_link_libraries(usb_midi_host_pio_example pico_cyw43_arch_none)
elseif(PICO_BOARD MATCHES adafruit_feather_rp2040_usb_host)
message("Board is Adafruit Feather RP2040 with USB A Host")
target_compile_definitions(usb_midi_host_pio_example PRIVATE USE_ADAFRUIT_FEATHER_RP2040_USBHOST=1)
elseif(PICO_BOARD MATCHES pico_w)
message("board is pico")
else()
message("board type is not tested; using pico settings")
endif()
else()
message("PICO_BOARD is not defined")
endif()

pico_add_extra_outputs(usb_midi_host_pio_example)

