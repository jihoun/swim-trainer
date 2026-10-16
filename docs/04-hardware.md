# Hardware

## Parts list

| Qty | Part | Notes | Approx. |
|---|---|---|---|
| 1 | **Seeed XIAO ESP32-S3** | Native USB-C (no driver on macOS), built-in LiPo charger | ₩10–15k |
| 2 | **HX711 breakout** | One per load cell. Switch to 80 Hz (see below) | ₩2–3k each |
| 2 | **S-type load cell, 50 kg** | Tension rated, threaded ends (M6/M8) | ₩15–25k each |
| 4 | Eye bolts matching the load cell thread | One each end of each cell | |
| 4 | Carabiners | Anchor ↔ cell ↔ band | |
| 1 | LiPo 3.7 V, 500–1000 mAh | JST-PH, solder to XIAO battery pads | ₩5–8k |
| 1 | Small enclosure | 3D-printed or a project box, hung at the door | |
| — | Shielded 4-core cable (~1 m ×2), headers, heat-shrink | Load cell → HX711 | |
| — | USB-C data cable | Charge-only cables are the #1 "Mac can't see board" cause | |

Bands: any swim stretch cords with paddles or handles. You'll calibrate each
one anyway.

## HX711 at 80 Hz

Most HX711 breakouts ship at 10 Hz (RATE pin tied to GND). For 80 Hz, tie RATE
to VCC: on SparkFun-style boards cut the RATE jumper; on the common green
boards lift pin 15 (RATE) from ground and wire it to VCC. Verify with the
firmware's `rate` command (it prints measured samples/s).

## Wiring

Both HX711s share 3.3 V and GND, each has its own DOUT/SCK pair.

| XIAO pin | GPIO | Connects to |
|---|---|---|
| 3V3 | — | HX711 L VCC, HX711 R VCC (and VDD if separate) |
| GND | — | HX711 L GND, HX711 R GND |
| D0 | GPIO1 | HX711 **L** DOUT |
| D1 | GPIO2 | HX711 **L** SCK |
| D2 | GPIO3 | HX711 **R** DOUT |
| D3 | GPIO4 | HX711 **R** SCK |

Load cell → HX711 (typical colour code, verify on your cell's label):

| Load cell wire | HX711 |
|---|---|
| Red (Exc+) | E+ |
| Black (Exc−) | E− |
| Green (Sig+) | A+ |
| White (Sig−) | A− |

If force reads negative when pulling, swap A+ and A−, or flip the sign in
`config.h` (`kSignLeft` / `kSignRight`).

Pins are defined in `firmware/src/config.h`.

## Mechanical chain

```
door anchor strap ─ carabiner ─ eye bolt ═ LOAD CELL ═ eye bolt ─ carabiner ─ band ─ paddle
```

- Put the electronics box at the door, cells close to it; keep the cables short
  and out of the band's path.
- **Safety**: a band or anchor failing under load whips back at your face. Use
  a rated door anchor, check carabiners are closed, inspect bands for nicks,
  and consider a backup strap that bypasses the load cell, slightly longer so
  it carries no load in normal use.
- Never exceed the load cell rating (50 kg ≈ 490 N, far above band forces).
