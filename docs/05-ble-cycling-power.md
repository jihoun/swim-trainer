# BLE: swim metrics as a cycling power meter

The ESP32 advertises the standard Bluetooth **Cycling Power Service (CPS,
0x1818)**. Anything that pairs with a power meter (Garmin, Wahoo, Zwift,
TrainerRoad, phone apps) will read it without custom software.

## Mapping

| CPS field | Swim meaning |
|---|---|
| Instantaneous Power (sint16, W) | Propulsive watts, both arms, 3 s window |
| Pedal Power Balance (uint8, ½ % units) + "reference = left" flag | Left share of work |
| Cumulative Crank Revolutions (uint16) | Completed stroke **cycles** (left + right) |
| Last Crank Event Time (uint16, 1/1024 s) | Time of last cycle completion |

Cadence is computed by the receiver from crank revolutions and event time,
so the watch shows **stroke cycles per minute**.

## Characteristics

| UUID | Name | Properties | Value |
|---|---|---|---|
| 0x2A63 | Cycling Power Measurement | Notify | See packet below |
| 0x2A65 | Cycling Power Feature | Read | `0x00000009` = bit 0 pedal power balance supported, bit 3 crank revolution data supported |
| 0x2A5D | Sensor Location | Read | `0` (Other) |

Also exposed: Device Information (0x180A) with manufacturer and model strings.

## Measurement packet (little-endian)

```
offset size field
0      2    flags = 0x0023
              bit0 pedal power balance present
              bit1 balance reference = left
              bit5 crank revolution data present
2      2    instantaneous power (sint16, W)
4      1    pedal power balance (uint8, 0.5 % units, left share)
5      2    cumulative crank revolutions (uint16)
7      2    last crank event time (uint16, 1/1024 s, wraps)
```

9 bytes, sent at 4 Hz. Only sent while a client is subscribed.

## Garmin specifics

- Pair under Sensors → Power. Use an **Indoor Bike** activity (or a custom
  "Swim Bench" activity based on it) so power and cadence fields are recorded.
- Garmin shows L/R balance only for power meters advertising balance, which
  this one does.
- Distance/speed will be zero; that's expected.

## Optional later: custom service

A vendor-specific service could stream per-stroke details (peak force, catch
rise time, per-stroke work) for a custom app. Not needed for v1.
