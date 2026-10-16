#pragma once
// Combines left and right work into windowed power, L/R balance and
// stroke-cycle counts (one cycle = one left + one right arm stroke).

#include <cstdint>

namespace swimpower {

enum class Side : uint8_t { Left = 0, Right = 1 };

class PowerCombiner {
 public:
  static constexpr uint32_t kBucketMs = 100;
  static constexpr uint32_t kBuckets = 30;  // 3 s window

  void addWork(Side side, uint32_t t_ms, float work_j);
  void onStroke(Side side, uint32_t t_ms);

  // Average power over the window ending at t_ms, watts.
  float powerW(uint32_t t_ms) const;
  // Left share of work over the window, percent (50 when no work).
  float leftBalancePct(uint32_t t_ms) const;

  uint32_t armStrokes() const { return arm_strokes_; }
  uint16_t cycles() const { return cycles_; }  // wraps, as BLE expects
  uint32_t lastCycleMs() const { return last_cycle_ms_; }

 private:
  struct Bucket {
    uint32_t id = UINT32_MAX;
    float work[2] = {0.0f, 0.0f};
  };
  void sums(uint32_t t_ms, float* left, float* right) const;

  Bucket buckets_[kBuckets];
  uint32_t arm_strokes_ = 0;
  uint16_t cycles_ = 0;
  uint32_t last_cycle_ms_ = 0;
};

}  // namespace swimpower
