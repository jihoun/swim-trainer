#pragma once
// Detects individual arm strokes from one side's force signal using
// hysteresis thresholds relative to a slowly tracked resting baseline.

#include <cstdint>

namespace swimpower {

struct StrokeConfig {
  float on_threshold_n = 20.0f;   // above baseline: stroke starts
  float off_threshold_n = 10.0f;  // above baseline: stroke ends
  uint32_t min_duration_ms = 250; // shorter pulses are ignored (noise, bumps)
  uint32_t refractory_ms = 200;   // dead time after a stroke ends
  float baseline_alpha = 0.01f;   // per-sample EMA weight while idle
};

struct Stroke {
  uint32_t start_ms = 0;
  uint32_t duration_ms = 0;
  uint32_t rise_ms = 0;   // start to peak force ("catch" speed hint)
  float peak_n = 0.0f;    // peak tension above zero, newtons
  float work_j = 0.0f;    // energy put into the band during the stroke
};

class StrokeDetector {
 public:
  explicit StrokeDetector(const StrokeConfig& cfg = StrokeConfig()) : cfg_(cfg) {}

  void setConfig(const StrokeConfig& cfg) { cfg_ = cfg; }
  const StrokeConfig& config() const { return cfg_; }

  // Feed one filtered sample plus the work added since the previous sample.
  // Returns true and fills `out` when a valid stroke has just completed.
  bool update(uint32_t t_ms, float force_n, float work_delta_j, Stroke* out);

  bool pulling() const { return state_ == State::Pulling; }
  float baseline() const { return baseline_; }

 private:
  enum class State { Idle, Pulling };

  StrokeConfig cfg_;
  State state_ = State::Idle;
  bool initialised_ = false;
  float baseline_ = 0.0f;
  uint32_t last_end_ms_ = 0;
  bool has_ended_ = false;
  Stroke current_;
  uint32_t peak_t_ms_ = 0;
};

}  // namespace swimpower
