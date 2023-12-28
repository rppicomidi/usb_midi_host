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
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "bsp/board_api.h"
#include "rppicomidi_USBH_MIDI.h"

USING_NAMESPACE_USBH_MIDI
USING_NAMESPACE_MIDI

static Rppicomidi_USBH_MIDI usbhMIDI;
static uint8_t midiDevAddr = 0;

/* MIDI IN MESSAGE REPORTING */
static void onMidiError(int8_t errCode)
{
    printf("MIDI Errors: %s %s %s\r\n", (errCode & (1UL << ErrorParse)) ? "Parse":"",
        (errCode & (1UL << ErrorActiveSensingTimeout)) ? "Active Sensing Timeout" : "",
        (errCode & (1UL << WarningSplitSysEx)) ? "Split SysEx":"");
}

static void onNoteOff(Channel channel, byte note, byte velocity)
{
    printf("C%u: Note off#%u v=%u\r\n", channel, note, velocity);
}

static void onNoteOn(Channel channel, byte note, byte velocity)
{
    printf("C%u: Note on#%u v=%u\r\n", channel, note, velocity);
}

static void onPolyphonicAftertouch(Channel channel, byte note, byte amount)
{
    printf("C%u: PAT#%u=%u\r\n", channel, note, amount);
}

static void onControlChange(Channel channel, byte controller, byte value)
{
    printf("C%u: CC#%u=%u\r\n", channel, controller, value);
}

static void onProgramChange(Channel channel, byte program)
{
    printf("C%u: Prog=%u\r\n", channel, program);
}

static void onAftertouch(Channel channel, byte value)
{
    printf("C%u: AT=%u\r\n", channel, value);
}

static void onPitchBend(Channel channel, int value)
{
    printf("C%u: PB=%d\r\n", channel, value);
}

static void onSysEx(byte * array, unsigned size)
{
    printf("SysEx:\r\n");
    unsigned multipleOf8 = size/8;
    unsigned remOf8 = size % 8;
    for (unsigned idx=0; idx < multipleOf8; idx++) {
        for (unsigned jdx = 0; jdx < 8; jdx++) {
            printf("%02x ", *array++);
        }
        printf("\r\n");
    }
    for (unsigned idx = 0; idx < remOf8; idx++) {
        printf("%02x ", *array++);
    }
    printf("\r\n");
}

static void onSMPTEqf(byte data)
{
    uint8_t type = (data >> 4) & 0xF;
    data &= 0xF;    
    static const char* fps[4] = {"24", "25", "30DF", "30ND"};
    switch (type) {
        case 0: printf("SMPTE FRM LS %u \r\n", data); break;
        case 1: printf("SMPTE FRM MS %u \r\n", data); break;
        case 2: printf("SMPTE SEC LS %u \r\n", data); break;
        case 3: printf("SMPTE SEC MS %u \r\n", data); break;
        case 4: printf("SMPTE MIN LS %u \r\n", data); break;
        case 5: printf("SMPTE MIN MS %u \r\n", data); break;
        case 6: printf("SMPTE HR LS %u \r\n", data); break;
        case 7:
            printf("SMPTE HR MS %u FPS:%s\r\n", data & 0x1, fps[(data >> 1) & 3]);
            break;
        default:
          printf("invalid SMPTE data byte %u\r\n", data);
          break;
    }
}

static void onSongPosition(unsigned beats)
{
    printf("SongP=%u\r\n", beats);
}

static void onSongSelect(byte songnumber)
{
    printf("SongS#%u\r\n", songnumber);
}

static void onTuneRequest()
{
    printf("Tune\r\n");
}

static void onMidiClock()
{
    printf("Clock\r\n");
}

static void onMidiStart()
{
    printf("Start\r\n");
}

static void onMidiContinue()
{
    printf("Cont\r\n");
}

static void onMidiStop()
{
    printf("Stop\r\n");
}

static void onActiveSense()
{
    printf("ASen\r\n");
}

static void onSystemReset()
{
    printf("SysRst\r\n");
}

static void onMidiTick()
{
    printf("Tick\r\n");
}

static void onMidiInWriteFail(uint8_t devAddr, uint8_t cable, bool fifoOverflow)
{
    if (fifoOverflow)
        printf("Dev %u cable %u: MIDI IN FIFO overflow\r\n", devAddr, cable);
    else
        printf("Dev %u cable %u: MIDI IN FIFO error\r\n", devAddr, cable);
}

static void registerMidiInCallbacks()
{
    auto intf = usbhMIDI.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
    if (intf == nullptr)
        return;
    intf->setHandleNoteOff(onNoteOff);                      // 0x80
    intf->setHandleNoteOn(onNoteOn);                        // 0x90
    intf->setHandleAfterTouchPoly(onPolyphonicAftertouch);  // 0xA0
    intf->setHandleControlChange(onControlChange);          // 0xB0
    intf->setHandleProgramChange(onProgramChange);          // 0xC0
    intf->setHandleAfterTouchChannel(onAftertouch);         // 0xD0
    intf->setHandlePitchBend(onPitchBend);                  // 0xE0
    intf->setHandleSystemExclusive(onSysEx);                // 0xF0, 0xF7
    intf->setHandleTimeCodeQuarterFrame(onSMPTEqf);         // 0xF1
    intf->setHandleSongPosition(onSongPosition);            // 0xF2
    intf->setHandleSongSelect(onSongSelect);                // 0xF3
    intf->setHandleTuneRequest(onTuneRequest);              // 0xF6
    intf->setHandleClock(onMidiClock);                      // 0xF8
    // 0xF9 as 10ms Tick is not MIDI 1.0 standard but implemented in the Arduino MIDI Library
    intf->setHandleTick(onMidiTick);                        // 0xF9
    intf->setHandleStart(onMidiStart);                      // 0xFA
    intf->setHandleContinue(onMidiContinue);                // 0xFB
    intf->setHandleStop(onMidiStop);                        // 0xFC
    intf->setHandleActiveSensing(onActiveSense);            // 0xFE
    intf->setHandleSystemReset(onSystemReset);              // 0xFF
    intf->setHandleError(onMidiError);

    auto dev = usbhMIDI.getDevFromDevAddr(midiDevAddr);
    if (dev == nullptr)
        return;
    dev->setOnMidiInWriteFail(onMidiInWriteFail);
}

/* CONNECTION MANAGEMENT */
static void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables)
{
    printf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
    midiDevAddr = devAddr;
    registerMidiInCallbacks();
}

static void onMIDIdisconnect(uint8_t devAddr)
{
    printf("MIDI device at address %u unplugged\r\n", devAddr);
    midiDevAddr = 0;
}


/* MAIN LOOP FUNCTIONS */

static void blinkLED(void)
{
    const uint32_t intervalMs = 1000;
    static uint32_t startMs = 0;

    static bool led_state = false;
    if ( board_millis() - startMs < intervalMs)
        return;
    startMs += intervalMs;

    led_state = !led_state;
    board_led_write(led_state);
}

static void sendNextNote()
{
    static uint8_t firstNote = 0x5b; // Mackie Control rewind
    static uint8_t lastNote = 0x5f; // Mackie Control stop
    static uint8_t offNote = lastNote;
    static uint8_t onNote = firstNote;
    // toggle NOTE On, Note Off for the Mackie Control channels 1-8 REC LED
    const uint32_t intervalMs = 1000;
    static uint32_t startMs = 0;
    auto intf = usbhMIDI.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
    if (intf == nullptr)
        return; // not connected
    
    if ( board_millis() - startMs < intervalMs)
        return; // not enough time
    startMs += intervalMs;
    intf->sendNoteOn(offNote++, 0, 1);
    intf->sendNoteOn(onNote++, 0x7f, 1);
    
    if (offNote > lastNote)
        offNote = firstNote;
    if (onNote > lastNote)
        onNote = firstNote;
}

/* APPLICATION STARTS HERE */

int main() {

    bi_decl(bi_program_description("A USB MIDI host example."));
    usbhMIDI.setAppOnConnect(onMIDIconnect);
    usbhMIDI.setAppOnDisconnect(onMIDIdisconnect);
    board_init();
    printf("Pico MIDI Host Example\r\n");
    tusb_init();
    
    while (1) {
        // Update the USB Host
        tuh_task();

        // Handle any incoming data; triggers MIDI IN callbacks
        usbhMIDI.readAll();
    
        // Do other processing that might generate pending MIDI OUT data
        sendNextNote();
    
        // Tell the USB Host to send as much pending MIDI OUT data as possible
        usbhMIDI.writeFlushAll();
    
        // Do other non-USB host processing
        blinkLED();
    }
    return 0; // Never gets here
}
