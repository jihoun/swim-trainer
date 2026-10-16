# Power model: from band force to propulsive watts

## The idea in one paragraph

A resistance band is a spring: its tension `F` depends only on how far it is
stretched, `F = f(x)`. Calibrate that curve once per band. Then a load cell at
the anchor, which measures `F`, also tells us the stretch `x = f⁻¹(F)`. From
`x(t)` we get hand speed along the band, `v = dx/dt`, and power `P = F · v`.
The work of one stroke is the energy put into the band:

```
W_stroke = ∫ F dx  (from stretch at catch to stretch at end of push)
         = E(x_end) − E(x_start),   where E(x) = ∫₀ˣ f(s) ds
```

So **force alone is enough**. No accelerometer or position sensor.

## Why this is a reasonable "propulsive" metric

- The load cell only sees tension **along the band**, which is roughly the
  swimming direction. Sideways sculling barely changes band length and
  contributes almost nothing, which is close to what "propulsive" means.
- Only the **loading phase** counts (stretch increasing). During recovery the
  band pulls the hand back; that energy is returned by the band, not produced
  by the swimmer, so it is ignored.
- Paddle mass and arm inertia are negligible at these forces.

## Caveats

- **Rubber hysteresis**: the unloading curve sits below the loading curve.
  Calibrate while **stretching** (loading curve), because that's what we
  integrate.
- **Bands drift** with age and temperature. Recalibrate monthly or when numbers
  look off. Store calibration per band (LVL1/2/3 per side).
- **Anchor angle**: if the band is far from horizontal, the measured tension
  overstates the horizontal component. Keep the anchor around chest height when
  lying on the bench.
- Numbers are **self-consistent**, not comparable with ZWIM's watts or anyone
  else's. Use them to track your own progress.

## Band model (firmware: `BandModel`)

Calibration is a table of `(x_i, F_i)` points, strictly increasing in both.
Between points `F` is linear, so:

- `stretchFromForce(F)`: inverse linear interpolation; clamps below the first
  point (slack band → `x = 0`) and extrapolates beyond the last point with the
  last segment's slope.
- `energyAt(x)`: exact integral of the piecewise-linear curve (trapezoids for
  full segments, a partial trapezoid for the last one).

Units: stretch in metres, force in newtons, energy in joules.

## Per-side signal pipeline (firmware: `SidePipeline`)

At each new sample (80 Hz per HX711):

1. **Low-pass filter** force (one-pole IIR, ~10 Hz cutoff) to tame HX711 noise.
2. `x = stretchFromForce(F)`.
3. If `x > x_prev` (loading): `ΔW = energyAt(x) − energyAt(x_prev)`, add to the
   side's work accumulator and to the current stroke.
4. Run the **stroke detector**.

## Stroke detection (firmware: `StrokeDetector`)

Hysteresis on force relative to a resting baseline `F_rest` (band pre-tension
with arms extended forward):

```
IDLE    ── F > F_rest + on_threshold ──▶ PULLING (track peak force, work)
PULLING ── F < F_rest + off_threshold ─▶ IDLE   → emit stroke if
                                                  duration ≥ min_duration
```

- `F_rest` is updated slowly while IDLE (exponential average), so it follows
  you if you shift on the bench.
- Defaults: `on = 20 N`, `off = 10 N`, `min_duration = 250 ms`,
  `refractory = 200 ms`. Tune with real data.
- Each stroke yields: side, start time, duration, peak force, work (J), and
  **impulse rise** (time from start to peak, a "catch" quality hint).

## Combining both sides (firmware: `PowerCombiner`)

- **Power**: the sum of ΔW from both sides over a **3 s sliding window**
  (30 × 100 ms buckets), divided by 3 s. Same smoothing a Garmin "3 s power"
  field would show.
- **Balance**: `W_left / (W_left + W_right)` over the same window.
- **Stroke cycles**: each completed stroke adds one arm stroke; a **cycle**
  = 2 arm strokes, reported as one crank revolution. Cadence shown on the watch
  = cycles/min (≈ half of "strokes per minute" as many swim apps count it).

## Sanity check on magnitudes

A band at ~80 N over a 0.6 m stretch stores roughly ½ · 80 · 0.6 ≈ 24 J per
stroke. At 30 cycles/min (60 arm strokes/min, one per second):
24 J/s ≈ **24 W** propulsive. Elite swimmers produce on the order of
100–200 W on swim ergometers; age-groupers far less. Expect double-digit watts.
