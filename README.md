# MIDI HOST DRIVER
This README file contains the design notes and limitations of the
MIDI host driver, and it describes how to build an run a simple
example to test it. The driver should run on any TinyUSB supported
processor with USB Host Bulk endpoint support, but the driver and
example code have only been tested on a RP2040 in a Raspberry Pi
Pico board.

# ACKNOWLEDGEMENTS
This driver code is based on code that rppicomidi submitted to TinyUSB
as pull request #1219. The pull request was never merged and got stale.
TinyUSB pull request #1627 by atoktoto started with pull request #1219,
but used simpler RP2040 bulk endpoint support from TinyUSB pull request
#1434. Pull request #1627 was reviewed by todbot, AndrewCapon, PaulHamsh,
and rppicomidi and was substantially functional. This driver copied
the `midi_host.c/h` files from pull request #1627 and renamed them
`usb_midi_host.c/h`. It also fixed some minor issues in the driver
API and added the application driver wrapper `usb_midi_host_app_driver.c`.
The example code code is adapted from TinyUSB pull request #1627. 

# OVERALL DESIGN, INTEGRATION
Although it is possible in the future for the TinyUSB stack to
incorporate this driver into its builtin driver support, right now
it is used as an application driver external to the stack. Installing
the driver to the TinyUSB stack requires adding it to the array
of application drivers returned by the `usbh_app_driver_get_cb()`
function. The `CMakeLists.txt` file contains two `INTERFACE` libraries.
If this driver is your only application USB host driver external
to the TinyUSB stack, you should install this driver by
adding the `usb_midi_host_app_driver` library to your main
application's `CMakeLists.txt` file's `target_link_libraries`.
If you want to add multiple host drivers, you must implement
your own `usbh_app_driver_get_cb()` function and you should
add the `usb_midi_host` library to your main application's
`CMakeLists.txt` file's `target_link_libraries` instead.

# EXAMPLE PROGRAM
Before you try to use this driver with TinyUSB, please make
sure the `usbh_app_driver_get_cb()` is supported in your
version of TinyUSB. This feature was introduced on 15-Aug-2023
with commit 7537985c080e439f6f97a021ce49f5ef48979c78. If you
are using the `pico-sdk` for the Raspberry Pi Pico, you
should be able to ensure you have the latest TinyUSB code
by pulling the latest master branch from the TinyUSB repo.

```
cd ${PICO_SDK_PATH}/lib/tinyusb
git checkout master
git pull
```

The example directory in this project contains a simple example
that plays a 5 note sequence on MIDI cable 0 and prints out
every MIDI message it receives. It is designed to run on a
Raspberry Pi Pico board. You will need a USB to UART adapter
to see the serial interface display. You will need a 5V
power source to provide power to the Pico board and the
connected USB MIDI device. I used a Pico board wired as a
Picoprobe to provide both the USB to UART adapter and the
5V power source. Finally, you will need a microUSB to USB A
adapter to allow you to connect your USB MIDI device to the
test system.
```
cd example
mkdir build
```
To build
```
cd build
cmake ..
make
```
To test, copy the UF2 file to the Pico board using whatever
method you prefer. Connect a microUSB to USB A adapter to
the Pico board USB connector. Connect the USB to UART adapter
to the Pico board pins 1 and 2. Connect the Ground pin of the
USB to UART adapter boardto a Ground pin of the Pico Board (I use
the Pico board debug port Ground pin). If it is different from
the ground pin of the USB to UART adapter, connect the Ground
pin of the 5V power source to a ground pin of the Pico board.
Connect the 5V pin of the 5V power source to pin 40 of the Pico
board. Make sure the board powers up and displays the message
```
Pico MIDI Host Example
```
before you attach your USB MIDI device.

Attach your USB MIDI device to the microUSB to USB A adapter
with the appropriate cable. You should see a message similar
to
```
MIDI device address = 1, IN endpoint 2 has 1 cables, OUT endpoint 1 has 1 cables
```
If your MIDI device can generate sound, you should start hearing a pattern
of notes from B-flat to D. If your device is Mackie Control compatible, then
the transport LEDs should sequence. If you use a control on your MIDI device, you
should see the message traffic displayed on the serial console.

# MAXIMUM NUMBER OF MIDI DEVICES ATTACHED TO HOST
You should define the value `CFG_TUH_DEVICE_MAX` in tusb_config.h to
match the number of USB hub ports attached to the USB host port. For
example
```
// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB ? 4 : 1) // hub typically has 4 ports
```

# MAXIMUM NUMBER OF ENDPOINTS
Although the USB MIDI 1.0 Class specification allows an arbitrary number
of endpoints, this driver supports at most one USB BULK DATA IN endpoint
and one USB BULK DATA OUT endpoint. Each endpoint can support up to 16 
virtual cables. If a device has multiple IN endpoints or multiple OUT
endpoints, it will fail to enumerate.

Most USB MIDI devices contain both an IN endpoint and an OUT endpoint,
but not all do. For example, some USB pedals only support an OUT endpoint.
This driver allows that.

# PUBLIC API
Applications interact with this driver via 8-bit buffers of MIDI messages
formed using the rules for sending bytes on a 5-pin DIN cable per the
original MIDI 1.0 specification.

To send a message to a device, the Host application composes a sequence
of status and data bytes in a byte array and calls the API function.
The arguments of the function are a pointer to the byte array, the number
of bytes in the array, and the target virtual cable number 0-15.

When the host driver receives a message from the device, the host driver
will call a callback function that the host application registers. This
callback function contains a pointer to a message buffer, a message length,
and the virtual cable number of the message buffer. One complete bulk IN
endpoint transfer might contain multiple messages targeted to different
virtual cables.

If you prefer, you may read and write raw 4-byte USB MIDI 1.0 packets.

# SUBCLASS AUDIO CONTROL
A MIDI device is supposed to have an Audio Control Interface, before
the MIDI Streaming Interface, but many commercial devices do not have one.
To support these devices, the descriptor parser in this driver will skip
past any audio control interface and audio streaming interface and open
only the MIDI interface.

An audio streaming host driver can use this driver by passing a pointer
to the MIDI interface descriptor that is found after the audio streaming
interface to the midih_open() function. That is, an audio streaming host
driver would parse the audio control interface descriptor and then the
audio streaming interface and endpoint descriptors. When the next descriptor
pointer points to a MIDI interface descriptor, call midih_open() with that
descriptor pointer.

# CLASS SPECIFIC INTERFACE AND REQUESTS
The host driver only makes use of the informaton in the class specific
interface descriptors to extract string descriptors from each IN JACK and
OUT JACK. To use these, you must set `CFG_MIDI_HOST_DEVSTRINGS` to 1 in
your application's tusb_config.h file. It does not parse ELEMENT items
for string descriptors.

This driver does not support class specific requests to control
ELEMENT items, nor does it support non-MIDI Streaming bulk endpoints.

# MIDI CLASS SPECIFIC DESCRIPTOR TOTAL LENGTH FIELD IGNORED
I have observed at least one keyboard by a leading manufacturer that
sets the wTotalLength field of the Class-Specific MS Interface Header
Descriptor to include the length of the MIDIStreaming Endpoint
Descriptors. This is wrong per my reading of the specification.

# MESSAGE BUFFER DETAILS
Messages buffers composed from USB data received on the IN endpoint will never
contain running status because USB MIDI 1.0 class does not support that. Messages
buffers to be sent to the device on the OUT endpont could contain running status
(the message might come from a UART data stream from a 5-pin DIN MIDI IN
cable on the host, for example). However, this driver does not correctly parse or
compose 4-byte USB MIDI Class packets from streams encoded with running status.
If this feature is important to you, please file an issue.

Message buffers to be sent to the device may contain real time messages
such as MIDI clock. Real time messages may be inserted in the message 
byte stream between status and data bytes of another message without disrupting
the running status. However, because MIDI 1.0 class messages are sent 
as four byte packets, a real-time message so inserted will be re-ordered
to be sent to the device in a new 4-byte packet immediately before the
interrupted data stream.

Real time messages the device sends to the host can only appear between
the status byte and data bytes of the message in System Exclusive messages
that are longer than 3 bytes.

# POORLY FORMED USB MIDI DATA PACKETS FROM THE DEVICE
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

# ENUMERATION FAILURES
The host may fail to enumerate a device if it has too many endpoints, if it has
if it has a Standard MS Transfer Bulk Data Endpoint Descriptor (not supported),
if it has a poorly formed descriptor, or if the descriptor is too long for
the host to read the whole thing.

