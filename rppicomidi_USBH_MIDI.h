/*
 * @file rppicomidi_USBH_MIDI.cpp
 * @brief Arduino MIDI Library compatible wrapper for usb_midi_host
 *        application driver
 *
 * This library manages all USB MIDI devices connected to the
 * single USB Root hub for TinyUSB for the whole plug in,
 * operate, unplug lifecycle. It makes each virtual MIDI cable for
 * each connected USB MIDI device behave as if it were a serial port
 * MIDI device and enables applications to use the Arduino MIDI Library
 * (formally called the FortySevenEffects MIDI library) to send and
 * receive MIDI messages between the application and the device.
 *
 * Most applications should only instantiate the Rppicomidi_USBH_MIDI
 * class by calling Rppicomidi_USBH_MIDI::instance(); e.g.
 *   auto usbhMIDI = Rppicomidi_USBH_MIDI::instance();
 *
 * Please see the CONFIGURATION section below to allow your application
 * to tailor the memory utilization of this class
 *
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
 */

#pragma once
/// See the comments in rppicomidi_USBH_MIDI_Config.h for
/// instructions how to tailor the memory requirements of
/// this library to your application.
#include "rppicomidi_USBH_MIDI_Config.h"

#include "rppicomidi_USBH_MIDI_Device.h"

#include "rppicomidi_USBH_MIDI_namespace.h"

BEGIN_RPPICOMIDI_USBH_MIDI_NAMESPACE


using ConnectCallback    = void (*)(uint8_t, uint8_t, uint8_t);
using DisconnectCallback = void (*)(uint8_t);

/// @brief This is the class your application should directly
/// instantiate. It tracks when MIDI devices are connected
/// and disconnected from the root. The Application should implement
/// the onConnect() and onDisconnect() methods to track the
/// USB devAddr value of each connected MIDI device.
///
/// This implementation only supports one USB MIDI host port
/// because that is all TinyUSB supports.
class Rppicomidi_USBH_MIDI {
public:
  Rppicomidi_USBH_MIDI() : appOnConnect{nullptr}, appOnDisconnect{nullptr} {
        instance = this;
        for (uint8_t idx = 0; idx < CFG_TUH_DEVICE_MAX; idx++) devAddr2DeviceMap[idx] = nullptr;
    }

  Rppicomidi_USBH_MIDI(Rppicomidi_USBH_MIDI const &) = delete;
  void operator=(Rppicomidi_USBH_MIDI const &) = delete;

  /// @brief test if a MIDI device with the given devAddr is connected
  /// @param devAddr the USB device address of the device to test
  /// @return true if the MIDI device is connected, false otherwise
  bool isConnected(uint8_t devAddr) { return getDevFromDevAddr(devAddr) != nullptr; }

  /// @brief get the number of virtual MIDI IN cables the device supports
  /// @param devAddr the USB device address of the MIDI device
  /// @return the number of MIDI IN virtual cables the connected device supports or
  /// 0 if the device at devAddr is no longer connected
  uint8_t getNumInCables(uint8_t devAddr) { auto ptr = getDevFromDevAddr(devAddr); return ptr != nullptr ? ptr->getNumInCables() : 0; }

  /// @brief get the number of virtual MIDI OUT cables the device supports
  /// @param devAddr the USB device address of the MIDI device
  /// @return the number of MIDI OUT virtual cables the connected device supports
  /// or 0 if the device at devAddr is no longer connected
  uint8_t getNumOutCables(uint8_t devAddr) { auto ptr = getDevFromDevAddr(devAddr); return ptr != nullptr ? ptr->getNumOutCables() : 0; }

  /// @brief Get a pointer to the MidiInterface object associated with a connected MIDI device
  /// @param devAddr the USB device address of a connected MIDI device
  /// @param cable the virtual cable number of the MIDI device the MidiInterface object supports
  /// @return a pointer to the MidiInterface object or nullptr if no object associated
  /// with the devAddr and cable exists (e.g., because the device has been disconnected)
  MIDI_NAMESPACE::MidiInterface<Rppicomidi_USBH_MIDI_Transport, MidiHostSettings>* getMIDIinterface(uint8_t devAddr, uint8_t cable) {
    auto ptr = getDevFromDevAddr(devAddr);
    return ptr != nullptr ? &(ptr->getMIDIinterface(cable)) : nullptr;
  }

  /// @brief Register a callback function to be called when a MIDI device
  /// is connected
  /// @param ftpr is a pointer to the callback function to be called
  void setAppOnConnect(ConnectCallback fptr) { appOnConnect = fptr; }

  /// @brief Unregister the last callback function that was registered 
  /// to be called when a MIDI device is connected
  void unsetAppOnConnect() { appOnConnect = nullptr; }

  /// @brief Register a callback function to be called when a MIDI device
  /// is connected
  /// @param ftpr is a pointer to the callback function to be called
  void setAppOnDisconnect(DisconnectCallback fptr) { appOnDisconnect = fptr; }

  /// @brief Unregister the last callback function that was registered 
  /// to be called when a MIDI device is disconnected
  void unsetAppOnDisconnect() { appOnConnect = nullptr; }

  /// @brief call the read method for every connected
  /// device's virtual MIDI IN cable. This will trigger the callback
  /// for that device.
  /// @return a bitmap such that bit i is set if cable i has a message ready
  uint16_t readAll() {
    uint16_t hasMessageBitmap = 0;
    for (uint8_t dev = 0; dev < RPPICOMIDI_TUH_MIDI_MAX_DEV; dev++) {
      uint8_t nCables = devices[dev].getNumInCables();
      uint16_t mask = 1;
      for (uint8_t cable = 0; cable < nCables; cable++) {
        if (devices[dev].getMIDIinterface(cable).read()) {
          hasMessageBitmap |= mask;
        }
        mask <<= 1;
      }
    }
    return hasMessageBitmap;
  }

  /// Send as many pending USB MIDI packets as possible to
  /// the connected MIDI devices
  void writeFlushAll() {
    for (uint8_t dev = 0; dev < RPPICOMIDI_TUH_MIDI_MAX_DEV; dev++) {
      devices[dev].writeFlush();
    }
  }

  /// @brief decode hasMessageBitmap returned by the readAll function to check
  /// if a particular virtual MIDI IN cable has a message waiting
  /// @param cable the virtual cable number
  /// @param hasMessageBitmap the value returned by readAll()
  /// @return true if there is a message waiting on the cable, false otherwise
  bool isMessageAvailableOnCable(uint8_t cable, uint16_t hasMessageBitmap) {
    return cable < RPPICOMIDI_TUH_MIDI_MAX_CABLES && (hasMessageBitmap & (1 << cable)) != 0;
  }

  /// @brief Get access to the Rppicomidi_USBH_MIDI_Device object associated with the devAddr
  /// @param devAddr the USB device address of the device
  /// @return a pointer to the associated Rppicomidi_USBH_MIDI_Device object or nullptr
  /// if there is no device attached to the devAddr
  Rppicomidi_USBH_MIDI_Device* getDevFromDevAddr(uint8_t devAddr) {
    if (devAddr == 0) // 0 is an unconfigured device
        return nullptr;
    uint8_t idx = 0;
    Rppicomidi_USBH_MIDI_Device* ptr = nullptr;
    for (; idx < RPPICOMIDI_TUH_MIDI_MAX_DEV && devAddr2DeviceMap[idx] != nullptr && devAddr2DeviceMap[idx]->getDevAddr() != devAddr; idx++) {}
    if (idx < RPPICOMIDI_TUH_MIDI_MAX_DEV && devAddr2DeviceMap[idx] != nullptr && devAddr2DeviceMap[idx]->getDevAddr() == devAddr) {
      ptr = devAddr2DeviceMap[idx];
    }
    return ptr;
  }

  /// @brief get a pointer to the MIDI Interface object associated with the USB device address
  /// and MIDI virtual IN cable number
  /// @param devAddr the USB device address
  /// @param cable the virtual MIDI IN cable number
  /// @return a pointer to the MIDI Interface object associated with the devAddr and cable
  /// or nullptr if no such interface exists (if, for example, the device was unplugged)
  MIDI_NAMESPACE::MidiInterface<Rppicomidi_USBH_MIDI_Transport, MidiHostSettings>* getInterfaceFromDeviceAndCable(uint8_t devAddr, uint8_t cable) {
    auto dev = getDevFromDevAddr(devAddr);
    if (dev != nullptr && cable < RPPICOMIDI_TUH_MIDI_MAX_CABLES && (cable < dev->getNumInCables() || cable < dev->getNumOutCables()))
      return  &dev->getMIDIinterface(cable);
    return nullptr;
  }

  // The following 3 functions should only be used by the tuh_midi_*_cb()
  // callback functions in file rppicomidi_USBH_MIDI.cpp.
  // They are declared public because the tuh_midi_*cb() callbacks are not
  // associated with any object and this class is accessed via the getInstance()
  // function.
  void onConnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables) {
    // try to allocate a Rppicomidi_USBH_MIDI_Device object for the connected device
    uint8_t idx = 0;
    for (; idx < RPPICOMIDI_TUH_MIDI_MAX_DEV && devAddr2DeviceMap[idx] != nullptr; idx++) {}
    if (idx < RPPICOMIDI_TUH_MIDI_MAX_DEV && devAddr2DeviceMap[idx] == nullptr) {
      devAddr2DeviceMap[idx] = devices + idx;
      devAddr2DeviceMap[idx]->onConnect(devAddr, nInCables, nOutCables);
      if (appOnConnect) appOnConnect(devAddr, nInCables, nOutCables);
    }
  }
  void onDisconnect(uint8_t devAddr) { 
    // find the Rppicomidi_USBH_MIDI_Device object allocated for this device
      auto ptr = getDevFromDevAddr(devAddr);
      if (ptr != nullptr) {
        ptr->onDisconnect(devAddr);
        if (appOnDisconnect)
           appOnDisconnect(devAddr);
      }
  }
  static Rppicomidi_USBH_MIDI* getInstance() {return instance; }
private:
  Rppicomidi_USBH_MIDI_Device devices[RPPICOMIDI_TUH_MIDI_MAX_DEV];
  ConnectCallback appOnConnect;
  DisconnectCallback appOnDisconnect;

  // devAddr2DeviceMap[idx] == a pointer to an address if device idx
  // has been connected or nullptr if not.
  // The problem this solves is RPPICOMIDI_TUH_MIDI_MAX_DEV < CFG_TUH_DEVICE_MAX
  Rppicomidi_USBH_MIDI_Device* devAddr2DeviceMap[CFG_TUH_DEVICE_MAX];

  /// instance is the unique object that is created for this class.
  /// It does not use the the well known thread safe singleton pattern
  /// (e.g., https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern)
  /// because this class is designed to be instantiated as a file static 
  /// object. Because the object is created before main() starts, there
  /// is no race condition danger. It allows you to access class methods
  /// with simple dot notation instead of classname::instance() symantics.
  static Rppicomidi_USBH_MIDI* instance;
};

END_RPPICOMIDI_USBH_MIDI_NAMESPACE
