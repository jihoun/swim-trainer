#include "BandModel.h"

namespace swimpower {

BandModel::BandModel(const BandPoint* points, size_t count) {
  count_ = count > kMaxPoints ? kMaxPoints : count;
  for (size_t i = 0; i < count_; ++i) pts_[i] = points[i];
  cumEnergy_[0] = 0.0f;
  for (size_t i = 1; i < count_; ++i) {
    const float dx = pts_[i].stretch_m - pts_[i - 1].stretch_m;
    cumEnergy_[i] =
        cumEnergy_[i - 1] + 0.5f * (pts_[i].force_n + pts_[i - 1].force_n) * dx;
  }
}

float BandModel::stretchFromForce(float f) const {
  if (!valid()) return 0.0f;
  if (f <= pts_[0].force_n) return pts_[0].stretch_m;
  size_t i = 1;
  while (i < count_ - 1 && f > pts_[i].force_n) ++i;
  // Segment [i-1, i]; for i == count_-1 this also extrapolates beyond the end.
  const BandPoint& a = pts_[i - 1];
  const BandPoint& b = pts_[i];
  const float df = b.force_n - a.force_n;
  if (df <= 0.0f) return a.stretch_m;
  return a.stretch_m + (f - a.force_n) * (b.stretch_m - a.stretch_m) / df;
}

float BandModel::forceAt(float x) const {
  if (!valid()) return 0.0f;
  if (x <= pts_[0].stretch_m) return pts_[0].force_n;
  size_t i = 1;
  while (i < count_ - 1 && x > pts_[i].stretch_m) ++i;
  const BandPoint& a = pts_[i - 1];
  const BandPoint& b = pts_[i];
  const float dx = b.stretch_m - a.stretch_m;
  if (dx <= 0.0f) return a.force_n;
  return a.force_n + (x - a.stretch_m) * (b.force_n - a.force_n) / dx;
}

float BandModel::energyAt(float x) const {
  if (!valid()) return 0.0f;
  if (x <= pts_[0].stretch_m) return 0.0f;
  size_t i = 1;
  while (i < count_ - 1 && x > pts_[i].stretch_m) ++i;
  const BandPoint& a = pts_[i - 1];
  const float fx = forceAt(x);
  return cumEnergy_[i - 1] + 0.5f * (a.force_n + fx) * (x - a.stretch_m);
}

}  // namespace swimpower
