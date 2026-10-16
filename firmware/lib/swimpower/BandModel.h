#pragma once
// Force/stretch model of one resistance band, built from a calibration table.
// Pure C++, no Arduino dependency, so it can be unit-tested on the host.

#include <cstddef>

namespace swimpower {

struct BandPoint {
  float stretch_m;  // band stretch beyond the "just taut" position, metres
  float force_n;    // measured tension at that stretch, newtons
};

class BandModel {
 public:
  static constexpr size_t kMaxPoints = 16;

  BandModel() = default;
  // Copies up to kMaxPoints points. Points must be strictly increasing in
  // both stretch and force (the loading curve of a band always is).
  BandModel(const BandPoint* points, size_t count);

  bool valid() const { return count_ >= 2; }

  // Inverse of the force curve. Clamps to the first point's stretch below
  // the first force; extrapolates with the last segment beyond the table.
  float stretchFromForce(float force_n) const;

  // Force at a stretch (forward model). Same clamping/extrapolation rules.
  float forceAt(float stretch_m) const;

  // Elastic energy stored between the first point's stretch and `stretch_m`,
  // in joules. Exact for a piecewise-linear force curve.
  float energyAt(float stretch_m) const;

 private:
  BandPoint pts_[kMaxPoints] = {};
  float cumEnergy_[kMaxPoints] = {};  // energy at each table point
  size_t count_ = 0;
};

}  // namespace swimpower
