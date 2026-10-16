#include "PowerCombiner.h"

namespace swimpower {

void PowerCombiner::addWork(Side side, uint32_t t_ms, float work_j) {
  if (work_j <= 0.0f) return;
  const uint32_t id = t_ms / kBucketMs;
  Bucket& b = buckets_[id % kBuckets];
  if (b.id != id) {
    b.id = id;
    b.work[0] = b.work[1] = 0.0f;
  }
  b.work[static_cast<uint8_t>(side)] += work_j;
}

void PowerCombiner::onStroke(Side /*side*/, uint32_t t_ms) {
  ++arm_strokes_;
  if (arm_strokes_ % 2 == 0) {
    ++cycles_;
    last_cycle_ms_ = t_ms;
  }
}

void PowerCombiner::sums(uint32_t t_ms, float* left, float* right) const {
  const uint32_t now = t_ms / kBucketMs;
  *left = *right = 0.0f;
  for (const Bucket& b : buckets_) {
    if (b.id == UINT32_MAX || b.id > now || now - b.id >= kBuckets) continue;
    *left += b.work[0];
    *right += b.work[1];
  }
}

float PowerCombiner::powerW(uint32_t t_ms) const {
  float l, r;
  sums(t_ms, &l, &r);
  return (l + r) / (kBuckets * kBucketMs / 1000.0f);
}

float PowerCombiner::leftBalancePct(uint32_t t_ms) const {
  float l, r;
  sums(t_ms, &l, &r);
  const float total = l + r;
  return total > 0.0f ? 100.0f * l / total : 50.0f;
}

}  // namespace swimpower
