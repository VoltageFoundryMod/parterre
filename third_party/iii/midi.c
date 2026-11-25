#include "tusb.h"

#include "midi.h"
#include "serial.h"

void midi_rx_noop(uint8_t data1, uint8_t data2, uint8_t data3) {
  (void)data1;
  (void)data2;
  (void)data3;
}
static midi_rx_callback_t rx_callback = &midi_rx_noop;

void midi_task() {
  uint8_t packet[4];
  while (tud_midi_available()) {
    tud_midi_packet_read(packet);
#if 0
        rx_callback(packet[0] & 0xf, packet[0] & 0xf0, packet[1], packet[2]);
#else
    // is this expected? or are we off by 1B?
    rx_callback(packet[1], packet[2], packet[3]);
#endif
  }
}

// midi_tx() always takes 3 bytes, so shorter messages come padded with
// zeros (a clock is F8 00 00, a program change C0 05 00). The stream
// parser would send each padding byte as a stray one-byte packet, which a
// host can read as data under running status, so only the bytes that
// belong to a message are written; sysex passes through until F7.
static uint8_t tx_data_left = 0;  // data bytes the current message still has
static bool tx_in_sysex = false;

static uint8_t tx_data_len(uint8_t status) {
  switch (status & 0xf0) {
    case 0xc0:
    case 0xd0: return 1;
    case 0xf0:
      if (status == 0xf2) return 2;                    // song position
      if (status == 0xf1 || status == 0xf3) return 1;  // time code, song select
      return 0;
    default: return 2;
  }
}

static bool tx_keep(uint8_t b) {
  if (b >= 0xf8) return true;  // real time: anywhere, even inside a message
  if (b >= 0x80) {
    if (b == 0xf7 && !tx_in_sysex) return false;  // a stray end of sysex
    tx_in_sysex = b == 0xf0;  // any other status byte ends a sysex too
    tx_data_left = tx_data_len(b);
    return true;
  }
  if (tx_in_sysex) return true;
  if (tx_data_left > 0) {
    tx_data_left--;
    return true;
  }
  return false;  // padding, or data without a status byte
}

void midi_tx(uint8_t data1, uint8_t data2, uint8_t data3) {
#if 1
  //-- stream interface does more work - buffering on cable streams, sysex
  //filter, etc
  static const uint8_t cable_number = 0;
  const uint8_t in[3] = {data1, data2, data3};
  uint8_t packet[3];
  uint32_t n = 0;
  for (int i = 0; i < 3; i++) {
    if (tx_keep(in[i])) packet[n++] = in[i];
  }
  if (n > 0) tud_midi_stream_write(cable_number, packet, n);
#else
  //-- seems more efficient for limited use cases
  // (i guess a trailing zero in this packet?)
  /// but actually, i've done something amiss and it does not work
  /// (maybe need to zero-pad first byte instead, as appears in RX)
  uint8_t packet[4] = {status | (channel & 0xf), data1, data2, 0};
  tud_midi_packet_write(packet);
#endif
}

void midi_set_rx_callback(midi_rx_callback_t callback) {
  rx_callback = callback;
}
