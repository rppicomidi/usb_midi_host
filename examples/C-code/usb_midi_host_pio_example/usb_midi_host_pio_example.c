/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/**
 * This demo program is designed to test the USB MIDI Host driver for a single USB
 * MIDI device connected to the USB Host port. It sends to the USB MIDI device the
 * sequence of half-steps from B-flat to D whose note numbers correspond to the
 * transport button LEDs on a Mackie Control compatible control surface. It also
 * prints to a UART serial port console the messages received from the USB MIDI device.
 *
 * This program works with a single USB MIDI device connected via a USB hub, but it
 * does not handle multiple USB MIDI devices connected at the same time.
 */
#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "bsp/board_api.h"
#include "pico/multicore.h"
#include "pio_usb.h"
#include "tusb.h"
#ifdef RASPBERRYPI_PICO_W
#include "pico/cyw43_arch.h"
#endif

static uint8_t midi_dev_idx = TUSB_INDEX_INVALID_8;

static void blink_led(void)
{
    static uint32_t previous_timestamp = {0};

    static bool led_state = false;

    uint32_t now = board_millis();
    
    int64_t diff = (int64_t)now - (int64_t)previous_timestamp;
    if (diff < 0)
        diff = -diff;
    if (diff > 1000) {
#ifdef RASPBERRYPI_PICO_W
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
#else
        board_led_write(led_state);
#endif
        led_state = !led_state;
        previous_timestamp = now;
    }
}

static void send_next_note(void)
{
    static uint8_t first_note = 0x5b; // Mackie Control rewind
    static uint8_t last_note = 0x5f; // Mackie Control stop
    static uint8_t message[6] = {
        0x90, 0x5f, 0x00,
        0x90, 0x5b, 0x7f,
    };
    // toggle NOTE On, Note Off for the Mackie Control channels 1-8 REC LED
    const uint32_t interval_ms = 1000;
    static uint32_t start_ms = 0;

    // Blink every interval ms
    if ( board_millis() - start_ms < interval_ms) {
        return; // not enough time
    }
    start_ms += interval_ms;

    uint32_t nwritten = 0;
    // Transmit the note message on the highest cable number
    uint8_t cable = tuh_midi_get_tx_cable_count(midi_dev_idx) - 1;
    nwritten = 0;
    nwritten += tuh_midi_stream_write(midi_dev_idx, cable, message, sizeof(message));
 
    if (nwritten != 0)
    {
        ++message[1];
        ++message[4];
        if (message[1] > last_note)
            message[1] = first_note;
        if (message[4] > last_note)
            message[4] = first_note;
    }
}


int main()
{
    board_init(); // must be called before tuh_init()
    tuh_init(BOARD_TUH_RHPORT);
    printf("Pico MIDI Host Example\r\n");
#ifdef RASPBERRYPI_PICO_W
    // The Pico W LED is attached to the CYW43 WiFi/Bluetooth module
    // Need to initialize it so the the LED blink can work
    // This must be called after tuh_init(). Waiting for core1 to
    // boot does this.
    if (cyw43_arch_init()) {
        printf("WiFi/Bluetooth module init for board LED failed");
        return -1;
    }
#endif
    while (1) {
        blink_led();
        tuh_task(); // tinyusb host task
        bool connected = midi_dev_idx != TUSB_INDEX_INVALID_8 && tuh_midi_mounted(midi_dev_idx);

        // device must be attached and have at least one endpoint ready to receive a message
        if (connected && tuh_midi_get_tx_cable_count(midi_dev_idx) >= 1) {
            send_next_note();
            // transmit any previously queued bytes (do this once per loop)
            tuh_midi_write_flush(midi_dev_idx);
        }
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data)
{
  printf("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, %u OUT cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (midi_dev_idx == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    midi_dev_idx = idx;
  }
  else {
    printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx)
{
  if (idx == midi_dev_idx) {
    midi_dev_idx = TUSB_INDEX_INVALID_8;
    printf("MIDI device idx = %d is unmounted\r\n", idx);
  }
  else {
    printf("Unused MIDI device address = %d is unmounted\r\n", idx);
  }
}

void tuh_midi_rx_cb(uint8_t dev_idx, uint32_t num_packets)
{
  if (midi_dev_idx == dev_idx) {
    if (num_packets != 0) {
      uint8_t cable_num;
      uint8_t buffer[48];
      while (1) {
        uint32_t bytes_read = tuh_midi_stream_read(dev_idx, &cable_num, buffer, sizeof(buffer));
        if (bytes_read == 0)
          return;
        printf("MIDI RX Cable #%u:", cable_num);
        for (uint32_t idx = 0; idx < bytes_read; idx++) {
          printf("%02x ", buffer[idx]);
        }
        printf("\r\n");
      }
    }
  }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes)
{
    (void)idx;
    (void)num_bytes;
}
