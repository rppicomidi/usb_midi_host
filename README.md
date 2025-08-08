# usb_midi_host
TinyUSB has implemented a driver that implements the same functionality
that this driver implements. Rather than maintain an out of tree driver,
I have decided to stop supporting my own driver. I am maintaining this
Git repository to show MIDI host example programs, Raspberry Pi Pico
family USB Host hardware examples, some guidance on the software API, and
to explain work-arounds and issues when it is helpful. The source code for
the original application USB Host driver is still included for the curious,
but please do not use it. You can find the current TinyUSB MIDI Host driver
in TinyUSB's source tree under `${TINY_USB_PATH}/src/class/midi/midi_host.*`

The examples directory include both C code and Arduino sketches that have
been updated to use the built-in TinyUSB USB MIDI Host Class driver.
If you have older applications that used older versions of this library,
please consider porting them to the new TinyUSB USB MIDI Host Class Driver's
API. Please pull the latest verions of TinyUSB when building MIDI applications.
Direct all issues you encounter with the USB system to the TinyUSB project.
I will continue to support issues you find with my example applications when
time permits.

The example code in this project should run on any processor that TinyUSB
supports with USB Host Bulk endpoints, but examples have only
been tested on what TinyUSB calls RP2040 family boards: Raspberry Pi Pico,
Pico W, Pico 2, and the Adafruit Feather RP2040 with USB A Host. See
the TinyUSB project for a list of what processors apply.

# Table of Contents
- [ACKNOWLEDGEMENTS](#acknowledgements)
- [BUILDING USB MIDI HOST APPLICATIONS](#building-usb-midi-host-applications)
- [HARDWARE](#hardware)
- [API](#api)
- [EXAMPLE PROGRAMS](#example-programs)
- [CONFIGURATION AND TROUBLESHOOTING](#configuration-and-troubleshooting)
- [About USB MIDI 1.0](#about-usb-midi-10)

# ACKNOWLEDGEMENTS
The application driver code is based on code that rppicomidi submitted
to TinyUSB as pull request #1219. The pull request was never merged and
got stale. TinyUSB pull request #1627 by atoktoto started with pull request
#1219, but used simpler RP2040 bulk endpoint support from TinyUSB pull request
#1434. Pull request #1627 was reviewed by todbot, AndrewCapon, PaulHamsh,
and rppicomidi and was substantially functional. This driver copied
the `midi_host.c/h` files from pull request #1627 and renamed them
`usb_midi_host.c/h`. It also fixed some minor issues in the driver
API and added the application driver wrapper `usb_midi_host_app_driver.c`.
The driver C example code code is adapted from TinyUSB pull request #1219.
All of this work was eventually ported into TinyUSB by hathach to
become the TinyUSB built-in driver.

# BUILDING USB MIDI HOST APPLICATIONS

## Building C/C++ Applications (for RP2040-based boards)

### Basic Environment Setup 
Before you attempt to build any C/C++ applications, be sure
you have the toolchain properly installed and the `pico-sdk`
installed. Please make sure you can build and
run the blink example found in the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).
Version 2.1 or later of the `pico-sdk` offers the best support
for this project.

### ${PICO_SDK_PATH}
If you are following Chapter 3 of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
you installed VS Code and the Official Raspberry Pi Pico VS Code extension.
The VS Code extension installed the `pico-sdk` in the
`${HOME}/.pico-sdk/sdk/2.1.0` directory. If you followed the manual
toolchain installation per Appendix C of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
then you installed the `pico-sdk` in `${HOME}/pico`.

### TinyUSB Library
You will also need to make sure that the lastest TinyUSB library is installed.
If there is nothing in the directory `${PICO_SDK_PATH}/lib/tinyusb`, or
the directory does not exist, please run the following commands
```
cd ${PICO_SDK_PATH}
git submodule update --recursive --init
```

Once the directory exists, please run the following commands
```
cd ${PICO_SDK_PATH}/pico-sdk/lib/tinyusb
git checkout master
git pull
```

### `Pico-PIO-USB` Library
If you are using the `Pico-PIO-USB` Library to implement the
USB Host hardware (see the [HARDWARE](#hardware) section, below),
you need to manually install the `Pico-PIO-USB` Library where
TinyUSB can find it.

At the time of this writing, the latest `Pico-PIO-USB` library is
version 0.7.2. It is not working very will with the example code.
If you are using PIO USB instead of the native RP2040 hardware,
I recommend using version 0.7.1 of the Pico-PIO-USB library. To install it:
```
mkdir -p ${PICO_SDK_PATH}/lib/tinyusb/hw/mcu/raspberry_pi
cd ${PICO_SDK_PATH}/lib/tinyusb/hw/mcu/raspberry_pi
git clone https://github.com/sekigon-gonnoc/Pico-PIO-USB.git
cd Pico-PIO-USB
git checkout 0.7.1
```

## Building Arduino Applications
The latest Adafruit TinyUSB Arduino Library code contains the
the TinyUSB built-in USB MIDI Host class driver code. However, as of this
writing, the Adafruit TinyUSB Arduino Library code does not
enable the USB MIDI Host support. You can fix this by editing
the Adafruit TinyUSB Arduino Library's config file
for your board. Find in the config file the lines
```
// max device support (excluding hub device): 1 hub typically has 4 ports
#define CFG_TUH_DEVICE_MAX (3 * CFG_TUH_HUB + 1)
```
and after them, add the following lines:
```
// enable MIDI Host
#define CFG_TUH_MIDI (CFG_TUH_DEVICE_MAX)
```

See the [config file section](#config-configuration-file) on how
to locate the config file.

Once you have modified the Adafruit TinyUSB Arduino Library's
configuration file,
include in your project the Adafruit TinyUSB Arduino Library,
and, if your host port hardware requires it, the Pico_PIO_USB
Library using the Arduino IDE Library Manager.

### Building Arduino applications for the RP2040
To build any Arduino application on the RP2040, you should
install the Earle Philhower [arduino-pico](https://github.com/earlephilhower/arduino-pico) core for the Arduino IDE.

You need to set up the board under the Tools Menu.
If you are using the `Pico-PIO-USB` Library to implement your
USB Host hardware:
```
Tools->CPU Speed->120 MHz (or 240 MHz (Overclock))
Tools->USB Stack->"Adafruit TinyUSB"
Tools->Debug Port->Serial
Tools->Optimize: You can choose any option except Small (-Os) (Standard). I generally use Optimize Even More (-O3)
```

If you are using the native RP2040 USB hardware to implement
your USB Host hardware, please configure the core as follows:
```
Tools->CPU Speed->133 MHz (or faster, if you wish)
Tools->USB Stack->"Adafruit TinyUSB Host"
Tools->Debug Port->Serial1
Tools->Optimize: (Choose anything)
```
NOTE: The USB Stack option "Adafruit TinyUSB Host" is not
available in the `arduino-pico` package 3.6.2 and earlier.
You must install version 3.6.3 or later to make this option
available.

# HARDWARE
The hardware examples provided here target the Raspberry Pi Pico
family of boards. The example programs have been tested on a Raspberry Pi Pico board,
a Pico W board, a Pico 2 board, and an Adafruit RP2040 Feather with USB A Host board.
The Pico boards, like most RP2040-based boards, do not ship with a
USB Host friendly connector, and the development environments
generally assume you are using the USB connector in Device mode
to provide power to the board, and to allow software update, to
provide a serial port interface for a serial console monitor, etc.
You will likely have to modify your board to add a USB Host interface
connector. The RP2040-based boards offer two approaches.

## Software-based USB Host Port: `Pico-PIO-USB` Library
The `Pico-PIO-USB` library, which works for both C/C++ and Arduino,
uses the RP2040 PIO 0 and the CPU core to to efficiently
bit-bang a full-speed USB host port on 2 GPIO pins. Adafruit makes
a [RP2040 board](https://www.adafruit.com/product/5723) that uses
this method for USB Host.

If you are not using the Adafruit or similar board, you need to
wire up something yourself. Wire a USB A jack to the GPIO and
power pins on the Pico board as follows:
```
Pico/Pico W board pin   USB A connector pin
23 (or any GND pin)  ->     GND
21 (GP16)            ->     D+  (via a 22 ohm resistor should improve things)
22 (GP17)            ->     D-  (via a 22 ohm resistor should improve things)
24 (GP18)            ->     TinyUSB drives this pin high to enable VBus on the Adafruit Feather board
40 (VBus)            ->     VBus (safer if it has current limiting on the pin)
```
I use a low-cost USB breakout board and solder the 22 ohm resistors to cut
traces on the D+ and D- lines. I leave GP18 unconnected.

TODO Insert photos of my setup here.

The main advantages of this approach are:
- Your board will have both a USB Device port and
a USB Host port, which, in an Arduino environment
especially, is convenient. For example, the Arduino
`Serial` object will work with the Serial Monitor
console, and firmware update via the Arduino IDE
is supported. 
- If you need both a USB MIDI Host port
and a USB MIDI Device port at the same time, you can
do it. See the [pico-usb-midi-filter](https://github.com/rppicomidi/pico-usb-midi-filter),
[pico-usb-midi-processor](https://github.com/rppicomidi/pico-usb-midi-processor),
and [midi2piousbhub](https://github.com/rppicomidi/midi2piousbhub) projects
for examples of this.
- You can buy
[off-the-shelf hardware](https://www.adafruit.com/product/5723)
already wired to support a Host port using this method.

The disadvantages of this approach are:
- The RP2040 clock must run at a multiple of 120MHz. This
  is a bit slower than the default of 133MHz.
- It consumes 2 GPIO pins
- It consumes the PIO 0 module
- It consumes a fair amount of CPU time
- It takes a bit more code storage space and RAM space.
- The Pico_PIO_USB library can conflict with the drivers for the
Pico W WiFi/Bluetooth module. To prevent the conflicts, please
initialze the TinyUSB library before the WiFi/Bluetooth module lbiraries.

## RP2040 Native USB Hardware
The RP2040 USB core natively supports a host mode that is
good enough for MIDI. The minimum modification to a Pico
board is to connect a USB OTG adapter to the Micro USB
B connector and add an external 5V power supply between
the VBus pin (pin 40 on the Pico board) and any ground pin
on the Pico board. As long as your 5V power supply is clean
and protected against short circuit, it should be OK. It is
how I test native hardware USB Host.

TODO Insert photo of my setup here.

The main advantages of this approach are
- It does not consume 3 GPIO pins
- It does not consume PIO 0
- It does not need the memory the Pico_PIO_USB library uses

The disadvantages of this approach are
- Serial port console now has to use a UART; you have to
provide external hardware to interface that UART to your
computer terminal software (e.g., a Picoprobe). In Arduino,
you have to use `Serial1` or `Serial2` UARTs for serial
monitor to work.
- Software update either requires you to unplug the OTG
connector and connect the RP2040 in flash drive mode,
or you have to use a CMSIS-DAP debugger like the Raspberry Pi
[debug probe](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html).
- Depending on how you want to mount the development board, adapting
the board's native USB connector to USB A may be harder than just
soldering to a few GPIO pins.
- There appears to be a [bug in the RP2040 USB controller
hardware](https://github.com/rppicomidi/usb_midi_host/issues/14)
that prevents connection with the Arturia Beatstep Pro.
Other MIDI hardware may have the same problem.

# API

## Connection Management
There are two connection management functions every application must implement:
- `tuh_midi_mount_cb()`
- `tuh_midi_unmount_cb()`

Each device connected to the USB Host, either directly, or through
a hub, is identified by its device address, which is an 8-bit number.
In addition, during enumeration, the host discovers how many virtual
MIDI IN cables and how many virtual MIDI OUT cables the device has.
When someone plugs a MIDI device to the USB Host, this driver will
call the `tuh_midi_mount_cb()` function so the application can save
the device address and virtual cable information.

When someone unplugs a MIDI device from the USB Host, this driver
will call the `tuh_midi_unmount_cb()` function so the application
can mark the previous device address as invalid.

## MIDI Message Communication
There is one function that every application that supports MIDI IN
must implement
- `tuh_midi_rx_cb()`

When the USB Host receives MIDI IN packets from the MIDI device,
this driver calls `tuh_midi_rx_cb()` to notify the application
that MIDI data is available. The application should read that
MIDI data as soon as possible.

There are two ways to handle USB MIDI messages:
- As 4-byte raw USB MIDI 1.0 packets
    - `tuh_midi_packet_read()`
    - `tuh_midi_packet_write()`

- As serial MIDI 1.0 byte streams
    - `tuh_midi_stream_read()`
    - `tuh_midi_stream_write()`

Both `tuh_midi_packet_write()` and `tuh_midi_stream_write()`
only write MIDI data to a queue. Once you are done writing
all MIDI messages that you want to send in a single
USB Bulk transfer (usually 64 bytes but sometimes only
8 bytes), you must call `tuh_midi_write_flush()`. 

The `examples` folder contains both C-Code and Arduino
code examples of how to use the API.

## Sending long sysex messages or lots of short ones
The `tuh_midi_write_flush()` function causes data to be sent out over
the USB host port. The RP2040 hardware has to send a complete endpoint
buffer to the attached device before more data can be sent. Due to the
TinyUSB HCD driver implementation and limitations of the
RP2040 chip, the Pico's USB host port is relatively slow. It can take a couple
of milliseconds for `tuh_midi_write_flush()` to complete because the system only
scans through the pending transfers once per millisecond. If you are sending
a lot of data, keep in mind that Each MIDI packet is 4 bytes, so most devices
can handle 16 packets can in one buffer. If you have a long sysex message, take
the length of a sysex message in bytes, including the starting 0xF0 and 0xF7,
multiply by 4, divide by 3, and round up. That will tell you how many packets
you need to send the sysex message.

You can reduce latency by packing the Host's OUT endpoint buffer up to the device's
OUT endpoint buffer size with data before flushing. Your best bet is to configure
the usb_midi_host driver to use larger buffers and call `tuh_midi_write_flush()`
only once in your main loop. Yes, you will have latency, but it is fine to break
up sysex messages across multiple USB packets. The application does not have to flush
for every write.

## Arduino MIDI Library API
This library API is designed to be relatively low level and is well
suited for applications that require the application to touch
all MIDI messages (for example, bridging and filtering). If you want
your application to use the Arduino MIDI library API
to access the devices attached to your USB MIDI host,
install the [EZ_USB_MIDI_HOST](https://github.com/rppicomidi/EZ_USB_MIDI_HOST) wrapper library in addition
to this library. See that repository for more information.

# EXAMPLE PROGRAMS

## Software
Each example program does the same thing:
- play a 5 note sequence on MIDI cable 0
- print out every MIDI message it receives on cable 0.

The only difference among them is whether they are C/C++
examples or Arduino examples, and whether they use native
rp2040 hardware (in directory with name `usb_midi_host_example`)
or the Pico_PIO_USB software USB Host (in directory with name
`usb_midi_host_pio_example`).

## Building C-Code Examples (For RP2040 family boards)
First, set up your environment for command line `pico-sdk`
program builds. If you are new to this, please see
the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) and build the blink example
before you try this.

Next, install the libraries as described in the [Building C/C++
Applications](#building-cc-applications) section.

### Command Line Build
To build via command line (see Appendix C of the `Getting Started Guide`);

```
cd examples/C-code/[example program directory name]
mkdir build
cd build
cmake ..
make
```
Note: if you are building the `usb_midi_host_pio_example` for
the Adafruit RP2040 Feather with USB
Type A Host board, you should replace `cmake ..` with
```
cmake -DPICO_BOARD=adafruit_feather_rp2040_usb_host ..
```
If you don't do this, then the board will work right after you
program it, and will not work on reset or reboot. If you are
using any other board other than a Pico board, change
`adafruit_feather_rp2040_usb_host` to the name of the
file for your board (without the `.h` extension)
found in `${PICO_SDK_PATH}/src/boards/include/boards`.

### VS Code Build
To build using VS Code, for Version 2.1.0 of the `pico-sdk`, import the project to VS Code.
1. Click the `Raspberry Pi Pico Project` icon in the left toolbar.
2. Click `Import Project`
3. Chenge the Location to point to the `examples/C-code/[example program directory name]` directory
4. Make sure Pico-SDK version is 2.1.0.
5. Choose the Debugger and any advanced options
6. Click Import. If you are not using a Pico board and your Raspberry Pi
   Pico Extenstion for VS Code is 0.15.2 or later, do this:
    1. Click the Click the `Raspberry Pi Pico Project` icon in the left toolbar.
    2. Under `Project` click `Switch Board`.
    3. Find your board in the board list and click on the name.
7. Click the CMake icon in the left toolbar. If you are not using a Pico board
    and your Raspberry Pi Pico Extenstion for VS Code is 0.15.1 or earlier, do this:
    1. On the `PROJECT STATUS` line of the CMAKE window, click the `Open CMake Tool Extenstions Settings` gear icon. You have to mouse over
    the `PROJECT STATUS` line for the icon to appear.
    2. In the new Settings tab that opened in the editor pane, click the `Workspace` tab.
    3. Scroll down to the `CMake:Configure Args` item and click the `Add Item` button.
    4. Enter `-DPICO_BOARD=[your board name goes here]` and click OK. The board name is the file name without
       extension from `${PICO_SDK_PATH}/src/boards/include/boards`. For example, if you are using a
       adafruit_feather_rp2040_usb_host board, you would enter `-DPICO_BOARD=adafruit_feather_rp2040_usb_host`
8. On the `PROJECT STATUS` line of the CMAKE window,
   select the `Delete Cache and Reconfigure` icon.
   You have to mouse over the `PROJECT STATUS` line for the icon to appear.
9. Under the `Configure` option, select the Pico Kit.
10. Choose whether you want `Debug`, `Release` or `RelWithDebugInfo`. The `MinSizeRel` option can cause issues, so do not choose it.
11. Click Build

If you are using an older version of the `pico-sdk`, then the project
is already set up for VS Code. Just use the VS Code `File` menu to
open the project. Select the toolchain when prompted, and use the
CMake icon on the left toolbar to build the code (see steps 7-11, above).

## Testing C-Code Examples
To test, first prepare your development board as described
in the [Hardware](#hardware) section of this document. Next,
copy the UF2 file to the Pico board using whatever
method you prefer. Make sure the board powers up and displays the message
```
Pico MIDI Host Example
```
before you attach your USB MIDI device.

Attach your USB MIDI device to the USB A connector your
hardware provides. You should see a message similar
to
```
MIDI Device Index = 0, MIDI device address = 1, 1 IN cables, 1 OUT cables
```
If your MIDI device can generate sound, you should start hearing a pattern
of notes from B-flat to D. If your device is Mackie Control compatible, then
the transport LEDs should sequence. If you use a control on your MIDI device, you
should see the message traffic displayed on the serial console.

NOTE: Unfortunately, the `pico-sdk` does not currently allow you to use the
USB device port for console I/O and the PIO USB host port at the same time.
You can install a [library](https://github.com/rppicomidi/cdc_stdio_lib)
that lets you do this for you own projects. In these examples,
all printf() output goes to the UART 0 serial port. If you want an
example that uses this library and uses the native USB port for both
MIDI device and console output, see the [midi2piousbhub](https://github.com/rppicomidi/midi2piousbhub) project.

## Building and Testing Arduino Examples
To build and run the Arduino examples, in the Arduino IDE,
use the Library Manager to install this library and accept
all of its dependencies. If your hardware requires it,
install the Pico PIO USB library too. Make sure to set the version of the Pico PIO USB library to 0.7.1.
Next, in the IDE, select File->Examples->usb_midi_host->arduino->[your example program name]

Use the Arduino IDE to build and run the code. Make sure to start a serial
monitor or else the code will appear to lock up.

Attach a MIDI device to the USB A port.
You should see something like this in the Serial Port Monitor (of course,
your connected MIDI device will likely be different).
```
IDI Device Index = 0, MIDI device address = 1, 1 IN cables, 1 OUT cables
Device attached, address = 1
  iManufacturer       1     KORG INC.
  iProduct            2     nanoKONTROL2
  iSerialNumber       0

```
If your MIDI device can generate sound, you should start hearing a pattern
of notes from B-flat to D. If your device is Mackie Control compatible, then
the transport LEDs should sequence. If you use a control on your MIDI device, you
should see the message traffic displayed on the Serial Port Monitor.

# CONFIGURATION AND TROUBLESHOOTING
In addition to this section, you might find
[this guide](https://github.com/rppicomidi/pico_usb_host_troubleshooting)
helpful.

## Config (Configuration) File
In C/C++, the config file for your project is called `tusb_config.h`.
It should be in the include path of your project.

In Arduino code, the config file is stored in the `libraries` directory of
your `Arduino` directory as the file
`libraries/Adafruit_TinyUSB/src/arduino/ports/${target}/tusb_config_${target}.h`,
where `${target}` is the processor name of the processor on the target
hardware. For example, for a Rapsberry Pi Pico board, the file is
`libraries/Adafruit_TinyUSB/src/arduino/ports/rp2040/tusb_config_rp2040.h`.
Sadly, any changes you make to the config file will disappear if you update
`Adafruit_TinyUSB_Library`.

## Size of the Enumeration Buffer
When the USB Host driver tries to enumerate a device, it reads the
USB descriptors into a byte buffer. By default, that buffer is 256 bytes
long. Complex MIDI devices, or device that also have audio interfaces,
tend to have much longer USB descriptors. If a device fails to enumerate,
locate the line in your config file that contains `#define CFG_TUH_ENUMERATION_BUFSIZE` and change the default 256 to something larger
(for example, 512).
## Debug Log
If a device fails to enumerate, a debug log printout may be helpful.
Debug log levels go from 0 (no debug logging) to 3 (very verbose). The
default log level is 0. The most direct way to set the debug level is to
define `CFG_TUSB_DEBUG` in your config file. For example, to set the log
level to 2, make sure your config file contains the lines
```
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 2
#endif
```
The conditional is in case you choose to change the debug level by
setting an environment variable.

## Maximum Number of MIDI Devices Attached to the Host
You should define the value `CFG_TUH_DEVICE_MAX` in the tuh_config.h file to
match the number of USB hub ports attached to the USB host port. For
example
```
// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB*3 + 1) // hub typically has 4 ports
```

## Maximum Number of USB Endpoints
Although the USB MIDI 1.0 Class specification allows an arbitrary number
of endpoints, the MIDI Host driver supports at most one USB BULK DATA IN endpoint
and one USB BULK DATA OUT endpoint. Each endpoint can support up to 16 
virtual cables. If a device has multiple IN endpoints or multiple OUT
endpoints, it will fail to enumerate.

Most USB MIDI devices contain both an IN endpoint and an OUT endpoint,
but not all do. For example, some USB pedals only support an IN endpoint.
This driver allows that.

## Maximum Number of Virtual Cables
A USB MIDI 1.0 Class message can support up to 16 virtual cables. The function
`tuh_midi_stream_write()` uses 6 bytes of data stored in an array in
an internal data structure to deserialize a MIDI byte stream to a
particular virtual cable. To properly handle all 16 possible virtual cables,
`CFG_TUH_DEVICE_MAX*16*6` data bytes are required. If the application
needs to save memory, in file `tusb_cfg.h` set `CFG_TUH_CABLE_MAX` to
something less than 16 as long as it is at least 1.

## Poorly Formed USB MIDI Data Packets from the Device
Some devices do not properly encode the code index number (CIN) for the
MIDI message status byte even though the 3-byte data payload correctly encodes
the MIDI message. This driver looks to the byte after the CIN byte to decide
how many bytes to place in the message buffer.

Some devices do not properly encode the virtual cable number. If the virtual
cable number in the CIN data byte of the packet is not less than bNumEmbMIDIJack
for that endpoint, then the host driver assumes virtual cable 0 and does not
report an error.

Some MIDI devices will always send back exactly wMaxPacketSize bytes on
every endpoint even if only one 4-byte packet is required (e.g., NOTE ON).
These devices send packets with 4 packet bytes 0. This driver ignores all
zero packets without reporting an error.

## Enumeration Failures
Some devices claim USB MIDI support but do not conform to the USB MIDI specification.
These devices require custom PC or Mac drivers in order to work correctly. Such
devices are considered to be not "class compliant." The TinyUSB MIDI Host driver
currently supports class compliant devices only. An example of a device that is
not class compliant is the Boss KATANA-100 MkII.

Some newer class compliant devices only support MIDI if they are connected to a high speed USB
host. Most microcontrollers that use this driver are only capable of supporting a full
speed USB host. Devices with this constraint will return a USB descriptor without
USB MIDI support if MIDI requires high speed USB. The Line6 Pod Go is an
example of a device that will not support MIDI when connected to a full speed
USB Host. Devices like this won't work with most microcontrollers.

For class compliant USB MIDI devices, the host may fail to enumerate a device if
it has too many endpoints, if it has if it has a Standard MS Transfer Bulk
Data Endpoint Descriptor (not supported),
if it has a poorly formed descriptor, or if the configuration
descriptor is too long for the host to read the whole thing.
The most common failure, though, is the descriptor is too long. See
[Size of the Enumeration Buffer](#size-of-the-enumeration-buffer) for
how to address that.

# About USB MIDI 1.0
The original MIDI specification was designed to carry a MIDI data stream
over a UART-based serial port at 31.25 kbps. USB MIDI 1.0 is a transport for
carrying the same MIDI messages between a USB Host and a USB Device. Unlike
the byte-serial transport of a UART serial port, USB sends data in packets,
which are groups of bytes. Understanding how this driver encodes a MIDI
data stream into USB packets will help you properly size data buffers
and should help you to understand the sizes returned from the driver's
MIDI stream read and write functions.

You can download the [USB Device Class Specification for MIDI Devices 1.0](https://www.usb.org/sites/default/files/midi10.pdf)
for free for all of the details. If you are a user of this driver, it will
probably help you to review the specification, especially section 4.
Key concepts:

- MIDI data flows between a USB MIDI Host and a USB MIDI Device using Bulk
endpoints. The path from Host to Device uses Bulk Out endpoints, and the
path from Device to Host uses Bulk In endponts.
- On a RP2040 or RP2350, USB MIDI is Full-speed (12Mbps).
- Full Speed Bulk endpoints are have a maximum payload of 64 bytes but can
be as small as 8 bytes. Some USB MIDI devices are designed for MIDI 2.0 high
speed and can support bulk transfers up to 512 bytes per packet,
but if you connect such a device to this USB 2.0 Full Speed host, then
data transfers will still be limited to 64 bytes.
- The serial MIDI byte data stream is encoded into 4-byte packets that move
on a Bulk endpoint data transfer.
    - The first byte in the packet is divided into two 4-bit values.
        - The most significant 4 bits encode a virtual cable number (CN) 0-15.
        When you plug a USB MIDI device to a computer and it shows multiple
        "MIDI Ports" for a single device, each "MIDI Port" is a virtual cable.
        - The least significant 4 bits contain a Code Index Number (CIN) 0-15.
        The CIN encodes how many of the remaining 3 bytes of the MIDI packet
        contain a whole MIDI message or just a part of a system exclusive message.
        For channel messages, the CIN is the same number as the most significant
        4 bits of the status byte.
    - The remaining 3 bytes in a packet can contain a 1-byte, 2-byte or
    3-byte MIDI message, or they can contain data bytes of an ongoing
    system exclusive message.
- Since devices support between 8 and 64 bytes per bulk endpoint transfer, 
USB MIDI devices can handle between 2 and 16 MIDI messages (or sections of
a system exclusive message) in one Bulk transfer packet.

The host learns the maximum number of bytes per Bulk endpoint transfer,
the number of virtual cables, the product name string, the string labels
for each virtual cable, etc. by reading the USB MIDI device's Device
Descriptor, Configuration Descriptor, and String descriptors during enumeration. The host
enumerates each device after the device is attached directly or via a hub.
Descriptors are just data structures that describe device capabilities in
a standard way. For more details, see the USB MIDI Device spec cited
above and chapter 9 of [The USB Specification version 2.0](https://www.usb.org/document-library/usb-20-specification).




