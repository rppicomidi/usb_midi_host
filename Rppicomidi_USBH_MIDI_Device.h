#pragma once
#include "MIDI.h"
#include "Rppicomidi_USBH_MIDI_Transport.h"

#include "rppicomidi_USBH_MIDI_namespace.h"
/// See the comments in rppicomidi_USBH_MIDI_Config.h for
/// instructions how to tailor the memory requirements of
/// this library to your application.
#include "rppicomidi_USBH_MIDI_Config.h"
BEGIN_RPPICOMIDI_USBH_MIDI_NAMESPACE


/// @brief This class models a connected USB MIDI device
/// Applications normally do not instantiate this class
/// Use the API for the Rppicomidi_USBH_MIDI class instead.
class Rppicomidi_USBH_MIDI_Device {
public:
  Rppicomidi_USBH_MIDI_Device() : devAddr{0}, nInCables{0}, nOutCables{0}, onMidiInWriteFail{nullptr},
    // Need to statically construct the interfaces array which is configurable
    // in length from 1 to 16. Sorry the following lines in the initialization
    // list is ugly
    interfaces{transports[0]
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 1
    , transports[1]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 2
    , transports[2]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 3
    , transports[3]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 4
    , transports[4]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 5
    , transports[5]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 6
    , transports[6]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 7
    , transports[7]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 8
    , transports[8]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 9
    , transports[9]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 10
    , transports[10]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 11
    , transports[11]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 12
    , transports[12]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 13
    , transports[13]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 14
    , transports[14]
  #endif
  #if RPPICOMIDI_TUH_MIDI_MAX_CABLES > 15
    , transports[15]
  #endif
  } {
    clearTransports();
  }

  /// Call this function to configure the MIDI interface objects
  /// associated with the device's virtual MIDI cables
  void onConnect(uint8_t devAddr_, uint8_t nInCables_, uint8_t nOutCables_) {
    if (devAddr_ > 0 && devAddr_ <= RPPICOMIDI_TUH_MIDI_MAX_DEV) {
        devAddr = devAddr_;
        nInCables = nInCables_;
        nOutCables = nOutCables_;
        clearTransports(); // make sure all transports are initialzed
        uint8_t maxCables = nInCables > nOutCables ? nInCables : nOutCables;
        for (uint8_t idx = 0; idx < maxCables; idx++) {
            transports[idx].setConfiguration(devAddr, idx, idx < nInCables, idx < nOutCables);
            interfaces[idx].begin(MIDI_CHANNEL_OMNI);
        }
    }
  }

  /// @brief Call this function to unconfigure all MIDI interface objects
  /// associated with the device's virtual MIDI cables
  /// @param devAddr_ is currently not used
  void onDisconnect(uint8_t devAddr_) {
    (void)devAddr_;
    clearTransports();
  }

  /// @brief  
  /// @return the device address for this device object
  uint8_t getDevAddr() { return devAddr; }

  /// @brief 
  /// @return the number of virtual MIDI IN cables for this device object 
  uint8_t getNumInCables() { return nInCables; }

  /// @brief 
  /// @return the number of virtual MIDI OUT cables for this device object 
  uint8_t getNumOutCables() { return nOutCables; }

  /// @brief Get the MIDI interface object associated with a particular virtual MIDI cable
  /// @param cable the virtual MIDI cable
  /// @return a reference to the MIDI interface object
  MIDI_NAMESPACE::MidiInterface<Rppicomidi_USBH_MIDI_Transport, MidiHostSettings>& getMIDIinterface(uint8_t cable) {
    return interfaces[cable];
  }

  /// @brief Enqueue message bytes to MIDI IN FIFO of a particular transport
  /// @param cable the virtual cable number of the MIDI interface on the device
  /// @param buffer points to an array of bytes to send
  /// @param nBytes the number of bytes in the buffer to send
  void writeToInFIFO(uint8_t cable, uint8_t* buffer, uint16_t nBytes) {
    if (cable < nInCables) {
      if (!transports[cable].writeToInFIFO(buffer, nBytes)) {
        if (onMidiInWriteFail != nullptr) {
          onMidiInWriteFail(devAddr, cable, transports[cable].inOverflow());
        }
      }
    }
  }

  /// @brief register a callback function that is called if the USB receive
  /// callback fails to write the received data to the FIFO
  /// @param fptr a pointer to the callback function; 
  void setOnMidiInWriteFail(void (*fptr)(uint8_t devAddr, uint8_t cable, bool fifoOverflow)) {onMidiInWriteFail = fptr; }

  /// @brief Send any queued bytes to the connected device
  /// if the host bus is ready to do it. Does nothing if
  /// there is nothing to send or if the host bus is busy
  void writeFlush() {
    if (devAddr != 0)
        tuh_midi_stream_flush(devAddr);
  }
private:
  void clearTransports() {
    for (uint8_t idx = 0; idx < RPPICOMIDI_TUH_MIDI_MAX_CABLES; idx++) {
        transports[idx].end();
    }
  }
  uint8_t devAddr;
  uint8_t nInCables;
  uint8_t nOutCables;
  void (*onMidiInWriteFail)(uint8_t devAddr, uint8_t cable, bool fifoOverflow);
  Rppicomidi_USBH_MIDI_Transport transports[RPPICOMIDI_TUH_MIDI_MAX_CABLES];
  MIDI_NAMESPACE::MidiInterface<Rppicomidi_USBH_MIDI_Transport, MidiHostSettings> interfaces[RPPICOMIDI_TUH_MIDI_MAX_CABLES];
};

END_RPPICOMIDI_USBH_MIDI_NAMESPACE
