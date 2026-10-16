// Host unit tests for lib/swimpower:  pio test -e native

#include <unity.h>

#include <cmath>

#include "BandModel.h"
#include "CpsPacket.h"
#include "PowerCombiner.h"
#include "SidePipeline.h"
#include "StrokeDetector.h"

using namespace swimpower;

namespace {

// Linear band: 100 N per metre (energy = 50 x^2).
const BandPoint kLinear[] = {{0.0f, 0.0f}, {0.5f, 50.0f}, {1.0f, 100.0f}};

void test_band_inverse_and_forward() {
  BandModel m(kLinear, 3);
  TEST_ASSERT_TRUE(m.valid());
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.25f, m.stretchFromForce(25.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.75f, m.stretchFromForce(75.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, m.stretchFromForce(-5.0f));   // slack clamps
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.2f, m.stretchFromForce(120.0f));  // extrapolates
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 30.0f, m.forceAt(0.3f));
}

void test_band_energy_linear() {
  BandModel m(kLinear, 3);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, m.energyAt(0.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 50.0f * 0.3f * 0.3f, m.energyAt(0.3f));
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 50.0f * 0.8f * 0.8f, m.energyAt(0.8f));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 50.0f * 1.2f * 1.2f, m.energyAt(1.2f));
}

void test_band_energy_nonlinear() {
  const BandPoint pts[] = {{0.0f, 0.0f}, {0.1f, 20.0f}, {0.3f, 40.0f}};
  BandModel m(pts, 3);
  // Segment 1: 0.5*20*0.1 = 1.0; segment 2: 0.5*(20+40)*0.2 = 6.0
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 7.0f, m.energyAt(0.3f));
  // Halfway in segment 2: F = 30, extra = 0.5*(20+30)*0.1 = 2.5
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 3.5f, m.energyAt(0.2f));
}

// Feeds one half-sine pull of `peak` newtons lasting `dur_ms`, at 80 Hz,
// followed by `rest_ms` of resting force.
template <typename F>
uint32_t feedPull(F&& fn, uint32_t t0, float base, float peak, uint32_t dur_ms, uint32_t rest_ms) {
  const uint32_t step = 12;  // ~80 Hz
  uint32_t t = t0;
  for (; t < t0 + dur_ms; t += step) {
    const float phase = static_cast<float>(t - t0) / dur_ms;
    fn(t, base + peak * std::sin(phase * 3.14159265f));
  }
  for (; t < t0 + dur_ms + rest_ms; t += step) fn(t, base);
  return t;
}

void test_stroke_detector_counts_and_work() {
  StrokeDetector d;
  int strokes = 0;
  Stroke last;
  uint32_t t = 0;
  // Settle baseline at 5 N.
  for (; t < 1000; t += 12) d.update(t, 5.0f, 0.0f, nullptr);
  for (int i = 0; i < 4; ++i) {
    t = feedPull(
        [&](uint32_t tt, float f) {
          Stroke s;
          if (d.update(tt, f, 0.01f, &s)) {
            ++strokes;
            last = s;
          }
        },
        t, 5.0f, 80.0f, 700, 500);
  }
  TEST_ASSERT_EQUAL(4, strokes);
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 85.0f, last.peak_n);
  TEST_ASSERT_GREATER_THAN(250u, last.duration_ms);
  TEST_ASSERT_TRUE(last.rise_ms > 200 && last.rise_ms < 500);
  TEST_ASSERT_TRUE(last.work_j > 0.0f);
}

void test_stroke_detector_ignores_short_bump() {
  StrokeDetector d;
  uint32_t t = 0;
  for (; t < 500; t += 12) d.update(t, 5.0f, 0.0f, nullptr);
  int strokes = 0;
  feedPull(
      [&](uint32_t tt, float f) {
        if (d.update(tt, f, 0.0f, nullptr)) ++strokes;
      },
      t, 5.0f, 40.0f, 120, 300);
  TEST_ASSERT_EQUAL(0, strokes);
}

void test_pipeline_work_matches_band_energy() {
  SidePipeline p(1.0f);  // no filtering, so we can check the physics exactly
  p.setBand(BandModel(kLinear, 3));
  float work = 0.0f;
  uint32_t t = 0;
  // Ramp 0 -> 60 N (x 0 -> 0.6 m) then back to 0.
  for (int i = 0; i <= 60; ++i, t += 12) work += p.update(t, static_cast<float>(i)).work_delta_j;
  for (int i = 60; i >= 0; --i, t += 12) work += p.update(t, static_cast<float>(i)).work_delta_j;
  // Only the loading phase counts: 50 * 0.6^2 = 18 J.
  TEST_ASSERT_FLOAT_WITHIN(1e-2f, 18.0f, work);
}

void test_combiner_power_balance_cycles() {
  PowerCombiner c;
  // 3 s of work: left 1 J per 100 ms, right 0.5 J per 100 ms.
  for (uint32_t t = 0; t < 3000; t += 100) {
    c.addWork(Side::Left, t, 1.0f);
    c.addWork(Side::Right, t, 0.5f);
  }
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 15.0f, c.powerW(2999));  // 45 J / 3 s
  TEST_ASSERT_FLOAT_WITHIN(1e-2f, 66.67f, c.leftBalancePct(2999));
  // Window slides: 6 s later with no work, power is zero.
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, c.powerW(9000));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 50.0f, c.leftBalancePct(9000));

  c.onStroke(Side::Left, 1000);
  c.onStroke(Side::Right, 2000);
  c.onStroke(Side::Left, 3000);
  TEST_ASSERT_EQUAL_UINT16(1, c.cycles());
  TEST_ASSERT_EQUAL_UINT32(2000, c.lastCycleMs());
  TEST_ASSERT_EQUAL_UINT32(3, c.armStrokes());
}

void test_cps_packet_layout() {
  uint8_t buf[kCpsPacketSize];
  const size_t n = encodeCpsMeasurement(buf, 123.4f, 52.5f, 0x0102, 1000);
  TEST_ASSERT_EQUAL(9, n);
  TEST_ASSERT_EQUAL_HEX8(0x23, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buf[1]);
  TEST_ASSERT_EQUAL_UINT8(123, buf[2]);
  TEST_ASSERT_EQUAL_UINT8(0, buf[3]);
  TEST_ASSERT_EQUAL_UINT8(105, buf[4]);  // 52.5 % in 0.5 % units
  TEST_ASSERT_EQUAL_HEX8(0x02, buf[5]);
  TEST_ASSERT_EQUAL_HEX8(0x01, buf[6]);
  const uint16_t evt = static_cast<uint16_t>(buf[7] | (buf[8] << 8));
  TEST_ASSERT_EQUAL_UINT16(1024, evt);  // 1000 ms = 1024/1024 s
  TEST_ASSERT_EQUAL_UINT16(0, toBleEventTime(64000));  // wraps at 64 s
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_band_inverse_and_forward);
  RUN_TEST(test_band_energy_linear);
  RUN_TEST(test_band_energy_nonlinear);
  RUN_TEST(test_stroke_detector_counts_and_work);
  RUN_TEST(test_stroke_detector_ignores_short_bump);
  RUN_TEST(test_pipeline_work_matches_band_energy);
  RUN_TEST(test_combiner_power_balance_cycles);
  RUN_TEST(test_cps_packet_layout);
  return UNITY_END();
}
