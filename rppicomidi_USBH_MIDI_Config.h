/*
 * @file rppicomidi_USBH_MIDI_Config.h
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

 /*
 * Each USB MIDI device can have up to 16 virtual MIDI cables. Each
 * TinyUSB host driver instance can have at most CFG_TUH_DEVICE_MAX
 * MIDI devices connected via a USB hub. The USB host does not know in
 * advance how many devices will be connected. It does not know how
 * many virtual MIDI cables a connected MIDI device has. Because
 * Arduino discourages the use of dynamic memory, the defaults of this
 * library assume there is a MIDI device connected all hub ports and
 * each connected device has 16 virtual MIDI cables.
 *
 * If this is overkill for your application, #define new values for
 * RPPICOMIDI_TUH_MIDI_MAX_DEV and RPPICOMIDI_TUH_MIDI_MAX_CABLES
 * before including this file
 *
 * Also, because the incoming USB MIDI IN packets from an attached
 * device could be from any virtual MIDI IN cable for that device,
 * this library separately buffers packets from each MIDI cable
 * to allow the Arduino MIDI Library Transport available() and read()
 * functions to work correctly. If the default size of the buffers
 * become an issue, the applicaton should #define
 * RPPICOMIDI_TUH_MIDI_IN_CABLE_BUF_SZ to a new value before including
 * this file
 */

#pragma once
#include "midi_Settings.h"
#include "midi_Namespace.h"
#include "rppicomidi_USBH_MIDI_namespace.h"

 /// Default maximum number of connected MIDI devices supported
 #define RPPICOMIDI_TUH_MIDI_MAX_DEV_DEFAULT CFG_TUH_DEVICE_MAX

#ifndef RPPICOMIDI_TUH_MIDI_MAX_DEV
#define RPPICOMIDI_TUH_MIDI_MAX_DEV RPPICOMIDI_TUH_MIDI_MAX_DEV_DEFAULT
#endif

/// Default maximum number of virtual MIDI cables supported
/// per connected MIDI device. All virtual cables from 0 to
/// (RPPICOMIDI_TUH_MIDI_MAX_CABLES - 1) will be mapped available
/// for communication via the API. Data from a virtual cable that
/// is not mapped is discarded when received. Data sent to an
/// unmapped virtual cable is discarded when an application
/// attempts to write it.
#define RPPICOMIDI_TUH_MIDI_MAX_CABLES_DEFAULT 16

#ifndef RPPICOMIDI_TUH_MIDI_MAX_CABLES
#define RPPICOMIDI_TUH_MIDI_MAX_CABLES RPPICOMIDI_TUH_MIDI_MAX_CABLES_DEFAULT
#endif

/// Virtual cable MIDI IN buffer size.
/// When this code's callback function reads a USB MIDI packet
/// from the usb_midi_hostdriver, it extracts the data it receives
/// to a buffer assigned to each device's virtual cable. This value
/// defines how many bytes are stored for each virtual cable. 
/// Do not define this value to anything larger than 64K bytes
/// because the receive queue code cannot handle buffers larger
/// than 64K bytes
#define RPPICOMIDI_TUH_MIDI_IN_BUF_SZ_DEFAULT 128

#ifndef RPPICOMIDI_TUH_MIDI_IN_CABLE_BUF_SZ
#define RPPICOMIDI_TUH_MIDI_IN_CABLE_BUF_SZ RPPICOMIDI_TUH_MIDI_IN_BUF_SZ_DEFAULT
#endif

#ifndef RPPICOMIDI_TUH_MIDI_MAX_SYSEX
#define RPPICOMIDI_TUH_MIDI_MAX_SYSEX 128
#endif

/// Because the MIDI Library send() and sendSysEx() libraries
/// will try to send every byte in the sysex message all at once,
/// and because the  USB transmitter system can only send as many
/// bytes at once as the endpoint can support (usually 64 bytes),
/// and because the write() method of the Transport class can't
/// report an error that the MIDI Library will do anything about,
/// the USB transmitter FIFO has to be able to buffer an entire
/// maximum size system exclusive message. The buffer must be a
/// multiple of 4 bytes. The sysex buffer from messages may not
/// have the F0 and F7 bytes, but the USB packet needs to send them.
#define CFG_TUH_MIDI_TX_BUFSIZE  (((((RPPICOMIDI_TUH_MIDI_MAX_SYSEX) + 2) / 3) + 1) * 4)
BEGIN_RPPICOMIDI_USBH_MIDI_NAMESPACE
struct MidiHostSettings : public MIDI_NAMESPACE::DefaultSettings
{
    /// Note Off is a different message than Note On with 0 velocity,
    ///    especially for Mackie Control protocol
    static const bool HandleNullVelocityNoteOnAsNoteOff = false;

    /// USB messages always come in packets, so 1 byte parsing is a bad match.
    /// However, if don't use 1 byte parsing, then it appears parse() can
    /// be called recursively RPPICOMIDI_TUH_MIDI_MAX_SYSEX times, which
    /// will likely cause issues.
    static const bool Use1ByteParsing = true;
    static const unsigned SysExMaxSize = RPPICOMIDI_TUH_MIDI_MAX_SYSEX;
};
END_RPPICOMIDI_USBH_MIDI_NAMESPACE
