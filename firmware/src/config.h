#pragma once
#include <Arduino.h>

#include "StrokeDetector.h"

// ---- Pins (Seeed XIAO ESP32-S3) ------------------------------------------
// See docs/img/wiring-one-side.svg: SCK on D0/D2 and DT on D1/D3 keeps the wires uncrossed.
constexpr uint8_t kPinLeftSck = D0;    // GPIO1
constexpr uint8_t kPinLeftDout = D1;   // GPIO2
constexpr uint8_t kPinRightSck = D2;   // GPIO3
constexpr uint8_t kPinRightDout = D3;  // GPIO4

// Flip to -1 if a side reads negative when pulling.
constexpr float kSignLeft = 1.0f;
constexpr float kSignRight = 1.0f;

// ---- Load cells -----------------------------------------------------------
// Default calibration factors (raw units per kg). Replaced by `cal` + `save`.
constexpr float kDefaultCalLeft = 21000.0f;
constexpr float kDefaultCalRight = 21000.0f;
constexpr float kGravity = 9.80665f;

// ---- Signal processing ----------------------------------------------------
// One-pole low-pass weight per sample: 0.44 ~= 10 Hz cutoff at 80 samples/s.
constexpr float kFilterAlpha = 0.44f;

inline swimpower::StrokeConfig defaultStrokeConfig() {
  swimpower::StrokeConfig c;
  c.on_threshold_n = 20.0f;
  c.off_threshold_n = 10.0f;
  c.min_duration_ms = 250;
  c.refractory_ms = 200;
  c.baseline_alpha = 0.01f;
  return c;
}

// ---- BLE ------------------------------------------------------------------
constexpr const char* kDeviceName = "SwimPower";
constexpr uint32_t kNotifyIntervalMs = 250;  // 4 Hz
