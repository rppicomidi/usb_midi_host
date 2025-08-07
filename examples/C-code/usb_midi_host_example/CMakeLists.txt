cmake_minimum_required(VERSION 3.13)

set(BOARD pico_sdk)
include(pico_sdk_import.cmake)

project(usb_midi_host_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
add_executable(usb_midi_host_example
    usb_midi_host_example.c
)

pico_enable_stdio_uart(usb_midi_host_example 1)

target_include_directories(usb_midi_host_example PRIVATE
 ${CMAKE_CURRENT_LIST_DIR}
)

target_link_options(usb_midi_host_example PRIVATE -Xlinker --print-memory-usage)
target_compile_options(usb_midi_host_example PRIVATE -Wall -Wextra)
target_link_libraries(usb_midi_host_example tinyusb_host tinyusb_board pico_stdlib)
if(DEFINED PICO_BOARD)
if(${PICO_BOARD} MATCHES "pico_w")
message("board is pico_w")
# add additional compile and link options
target_link_libraries(usb_midi_host_example pico_cyw43_arch_none)
elseif(${PICO_BOARD} MATCHES "adafruit_feather_rp2040_usb_host")
message("Adding Pico PIO USB library for Adafruit Feather RP2040 USB Host board")
target_compile_definitions(usb_midi_host_example PRIVATE RPPICOMIDI_ADAFRUIT_FEATHER_RP2040_USB_HOST)
target_link_libraries(midi2usbhost tinyusb_pico_pio_usb)
else()
message("board is pico")
endif()
else()
message("board is not defined")
endif()

pico_add_extra_outputs(usb_midi_host_example)

