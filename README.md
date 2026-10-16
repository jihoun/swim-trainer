# swim-trainer

A DIY dryland swim trainer with a **swim power meter**: lie on a bench, pull on
resistance bands, and get live **propulsive watts, stroke rate and left/right
balance** on a Garmin watch, Zwift or any app that reads a bike power meter.

Inspired by [ZWIM](https://www.zwim.com) (ex-ZEN8), which ships only to the UK
and US. This project rebuilds the idea with off-the-shelf parts available in
Korea, plus an ESP32 that pretends to be a dual-sided cycling power meter.

## How it works

```
 door anchor
   ├─ load cell L ─ band L ─ paddle L ─┐
   └─ load cell R ─ band R ─ paddle R ─┤  swimmer lying on the bench
                                       │
 load cells → 2× HX711 → ESP32 → BLE Cycling Power Service (0x1818)
                                   ├─ Instantaneous power  = propulsive watts
                                   ├─ Pedal power balance  = left/right split
                                   └─ Crank revolutions    = stroke cycles (cadence)
```

The key idea: a band's tension depends only on how far it is stretched. Once
each band is calibrated (force vs stretch), the force signal alone gives the
stretch, the hand speed along the band and the work done per stroke. No
motion sensor is needed. See [docs/03-power-model.md](docs/03-power-model.md).

## Repository layout

| Path | What's in it |
|---|---|
| [docs/01-background.md](docs/01-background.md) | ZWIM / ZEN8 reference product and why this project exists |
| [docs/02-bench.md](docs/02-bench.md) | Bench requirements and what to buy in Korea |
| [docs/03-power-model.md](docs/03-power-model.md) | Physics: force → stretch → work → watts; stroke detection |
| [docs/04-hardware.md](docs/04-hardware.md) | Parts list, wiring, assembly, safety |
| [docs/05-ble-cycling-power.md](docs/05-ble-cycling-power.md) | How swim metrics map onto the BLE Cycling Power profile |
| [docs/06-dev-setup.md](docs/06-dev-setup.md) | macOS toolchain, flashing, calibration workflow |
| [firmware/](firmware/) | PlatformIO project for the Seeed XIAO ESP32-S3 |
| [firmware/lib/swimpower/](firmware/lib/swimpower/) | Hardware-independent power math (unit-tested on the host) |
| [tools/calibrate_band.py](tools/calibrate_band.py) | Fits a band's force/stretch table from measurements |

## Status

- [x] Concept, physics model and BLE mapping
- [x] Firmware skeleton: dual HX711, band model, stroke detection, BLE power meter
- [x] Host unit tests for the power math
- [ ] Hardware ordered and assembled
- [ ] Load cells calibrated, bands calibrated
- [ ] Stroke detection thresholds tuned on real pulls
- [ ] Paired with Garmin / Zwift

## Quick start (once the hardware is here)

```bash
cd firmware
pio run -t upload          # flash over USB-C
pio device monitor         # serial console: commands, live CSV
```

Then follow the calibration steps in [docs/06-dev-setup.md](docs/06-dev-setup.md).
