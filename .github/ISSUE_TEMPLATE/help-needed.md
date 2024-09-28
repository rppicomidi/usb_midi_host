---
name: Help Needed
about: Request help using this driver
title: "[Help Wanted]"
labels: ''
assignees: ''

---

**What help do you need?**
Ask a specific question here. If you can't be specific, at least describe in detail what you are trying to do.

**What have you tried that didn't do what you expected?**
List here what you tried, what you expected to happen, and what actually happened.

**What is your setup like?**
- Arduino or C/C++
- Target hardware board (Raspberry Pi Pico, Raspberry Pi Pico W, ...)
- What is your build machine (Windows PC, Linux PC, Mac)
- What build machine OS and version (Windows 11, MacOS Monterey,  Ubuntu Linux 22.04LTS, ...)
- Version of this library
- Version of build software (pico-sdk 1.5.1, Arduino IDE 2.3.0, Raspberry Pi Pico/RP2040 3.7, ...)
- How are you loading build images to the target (Arduino IDE+USB cable, Visual Studio Code+picoprobe, ...)

**Attach any additional information to help explain the issue**
Attach log files, screenshots, etc. If your issue is "Some of my devices work
correctly, but this one device does not," I will not be able to help you unless
you attach a file containing the USB descriptor of the device and a LOG=2 output
from the TinyUSB stack. See https://docs.tinyusb.org/en/latest/reference/getting_started.html
and https://github.com/rppicomidi/pico_usb_host_troubleshooting. Search the
web for tutorials if you do not know how to get serial port output from the
RP2040's UART0 (Pins 1 and 2 of the Pico board).
