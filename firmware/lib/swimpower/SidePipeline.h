#pragma once
// One side (left or right): raw force -> filtered force -> stretch -> work,
// plus stroke detection.

#include <cstdint>

#include "BandModel.h"
#include "StrokeDetector.h"

namespace swimpower {

struct SideResult {
  float force_n = 0.0f;       // filtered force
  float stretch_m = 0.0f;     // band stretch from the model
  float work_delta_j = 0.0f;  // positive work added by this sample (>= 0)
  bool stroke_done = false;
  Stroke stroke;              // valid when stroke_done
};

class SidePipeline {
 public:
  // filter_alpha: one-pole low-pass weight per sample (0..1].
  // 0.44 gives ~10 Hz cutoff at 80 samples/s.
  explicit SidePipeline(float filter_alpha = 0.44f) : alpha_(filter_alpha) {}

  void setBand(const BandModel& band) {
    band_ = band;
    reset();
  }
  void setStrokeConfig(const StrokeConfig& cfg) { detector_.setConfig(cfg); }
  const StrokeDetector& detector() const { return detector_; }

  void reset() {
    primed_ = false;
    filtered_ = 0.0f;
    prev_energy_ = 0.0f;
  }

  SideResult update(uint32_t t_ms, float raw_force_n) {
    SideResult r;
    if (!primed_) {
      filtered_ = raw_force_n;
    } else {
      filtered_ += alpha_ * (raw_force_n - filtered_);
    }
    r.force_n = filtered_;
    r.stretch_m = band_.stretchFromForce(filtered_);
    const float energy = band_.energyAt(r.stretch_m);
    if (primed_ && energy > prev_energy_) r.work_delta_j = energy - prev_energy_;
    prev_energy_ = energy;
    primed_ = true;
    r.stroke_done = detector_.update(t_ms, filtered_, r.work_delta_j, &r.stroke);
    return r;
  }

 private:
  float alpha_;
  BandModel band_;
  StrokeDetector detector_;
  bool primed_ = false;
  float filtered_ = 0.0f;
  float prev_energy_ = 0.0f;
};

}  // namespace swimpower
