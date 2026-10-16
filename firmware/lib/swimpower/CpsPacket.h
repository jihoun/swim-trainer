#pragma once
// Encodes a BLE Cycling Power Measurement (0x2A63) packet.
// Layout (little-endian), 9 bytes:
//   flags u16 = 0x0023 (balance present | balance ref = left | crank data)
//   instantaneous power s16 (W)
//   pedal power balance u8 (0.5 % units, left share)
//   cumulative crank revolutions u16
//   last crank event time u16 (1/1024 s)

#include <cstddef>
#include <cstdint>

namespace swimpower {

constexpr size_t kCpsPacketSize = 9;
constexpr uint16_t kCpsFlags = 0x0023;
// Cycling Power Feature (0x2A65): pedal power balance + crank revolution data.
constexpr uint32_t kCpsFeature = 0x00000009;

// Converts milliseconds to the wrapping 1/1024 s event time used by BLE.
inline uint16_t toBleEventTime(uint32_t t_ms) {
  return static_cast<uint16_t>((static_cast<uint64_t>(t_ms) * 1024u / 1000u) & 0xFFFFu);
}

inline size_t encodeCpsMeasurement(uint8_t* buf, float power_w, float left_balance_pct,
                                   uint16_t cycles, uint32_t last_cycle_ms) {
  int32_t p = static_cast<int32_t>(power_w + 0.5f);
  if (p < 0) p = 0;
  if (p > 32767) p = 32767;
  float bal = left_balance_pct;
  if (bal < 0.0f) bal = 0.0f;
  if (bal > 100.0f) bal = 100.0f;
  const uint8_t bal_half = static_cast<uint8_t>(bal * 2.0f + 0.5f);
  const uint16_t evt = toBleEventTime(last_cycle_ms);

  buf[0] = kCpsFlags & 0xFF;
  buf[1] = kCpsFlags >> 8;
  buf[2] = static_cast<uint8_t>(p & 0xFF);
  buf[3] = static_cast<uint8_t>((p >> 8) & 0xFF);
  buf[4] = bal_half;
  buf[5] = cycles & 0xFF;
  buf[6] = cycles >> 8;
  buf[7] = evt & 0xFF;
  buf[8] = evt >> 8;
  return kCpsPacketSize;
}

}  // namespace swimpower
