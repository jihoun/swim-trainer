#include "StrokeDetector.h"

namespace swimpower {

bool StrokeDetector::update(uint32_t t, float f, float dW, Stroke* out) {
  if (!initialised_) {
    baseline_ = f;
    initialised_ = true;
  }

  if (state_ == State::Idle) {
    const bool in_refractory = has_ended_ && (t - last_end_ms_) < cfg_.refractory_ms;
    if (!in_refractory && f > baseline_ + cfg_.on_threshold_n) {
      state_ = State::Pulling;
      current_ = Stroke();
      current_.start_ms = t;
      current_.peak_n = f;
      current_.work_j = dW > 0.0f ? dW : 0.0f;
      peak_t_ms_ = t;
    } else if (f < baseline_ + cfg_.off_threshold_n) {
      // Only follow the baseline when clearly at rest.
      baseline_ += cfg_.baseline_alpha * (f - baseline_);
    }
    return false;
  }

  // Pulling
  if (dW > 0.0f) current_.work_j += dW;
  if (f > current_.peak_n) {
    current_.peak_n = f;
    peak_t_ms_ = t;
  }
  if (f < baseline_ + cfg_.off_threshold_n) {
    state_ = State::Idle;
    has_ended_ = true;
    last_end_ms_ = t;
    current_.duration_ms = t - current_.start_ms;
    current_.rise_ms = peak_t_ms_ - current_.start_ms;
    if (current_.duration_ms >= cfg_.min_duration_ms) {
      if (out) *out = current_;
      return true;
    }
  }
  return false;
}

}  // namespace swimpower
