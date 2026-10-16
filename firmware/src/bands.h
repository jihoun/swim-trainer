#pragma once
// Band calibration tables: force (N) vs stretch (m) on the LOADING curve.
// The values below are PLACEHOLDERS shaped like a typical latex swim cord.
// Replace them with the output of tools/calibrate_band.py for your bands.

#include "BandModel.h"

namespace bands {

using swimpower::BandPoint;

// Light cord (LVL1-like)
constexpr BandPoint kLevel1[] = {
    {0.00f, 0.0f}, {0.10f, 14.0f}, {0.20f, 24.0f}, {0.30f, 32.0f},
    {0.40f, 39.0f}, {0.50f, 46.0f}, {0.60f, 53.0f}, {0.80f, 68.0f},
};

// Medium cord (LVL2-like)
constexpr BandPoint kLevel2[] = {
    {0.00f, 0.0f}, {0.10f, 22.0f}, {0.20f, 38.0f}, {0.30f, 50.0f},
    {0.40f, 61.0f}, {0.50f, 72.0f}, {0.60f, 83.0f}, {0.80f, 106.0f},
};

// Heavy cord (LVL3-like)
constexpr BandPoint kLevel3[] = {
    {0.00f, 0.0f}, {0.10f, 32.0f}, {0.20f, 55.0f}, {0.30f, 72.0f},
    {0.40f, 88.0f}, {0.50f, 103.0f}, {0.60f, 118.0f}, {0.80f, 150.0f},
};

struct BandSet {
  const char* name;
  const BandPoint* left;
  size_t left_count;
  const BandPoint* right;
  size_t right_count;
};

// Use separate tables per side once each physical band is calibrated.
constexpr BandSet kSets[] = {
    {"LVL1", kLevel1, sizeof(kLevel1) / sizeof(BandPoint), kLevel1, sizeof(kLevel1) / sizeof(BandPoint)},
    {"LVL2", kLevel2, sizeof(kLevel2) / sizeof(BandPoint), kLevel2, sizeof(kLevel2) / sizeof(BandPoint)},
    {"LVL3", kLevel3, sizeof(kLevel3) / sizeof(BandPoint), kLevel3, sizeof(kLevel3) / sizeof(BandPoint)},
};
constexpr size_t kSetCount = sizeof(kSets) / sizeof(kSets[0]);
constexpr size_t kDefaultSet = 1;  // LVL2

}  // namespace bands
