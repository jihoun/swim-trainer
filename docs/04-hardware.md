# Hardware

## Parts list

Coupang links and prices checked on 2026-10-16. Listings change; if one is
gone, search the Korean term in the last column.

| Qty | Part | Coupang | Price | Notes / search term |
|---|---|---|---|---|
| 1 | **Seeed XIAO ESP32-S3** | [9233076446](https://www.coupang.com/vp/products/9233076446) | ₩18,900 | Genuine Seeed board, native USB-C, LiPo charger. Ships ~11/3 |
| 2 | **HX711 breakout** | [8449606915](https://www.coupang.com/vp/products/8449606915) | ₩3,480 each | 24-bit load cell amp. Switch to 80 Hz (see below). *HX711 모듈* |
| 2 | **S-type load cell, 0–50 kg, M8×1.25** | [8863760189](https://www.coupang.com/vp/products/8863760189) | ₩43,640 each | Pick the "0-50kg M8x1.25" option. Alt: [9585874336](https://www.coupang.com/vp/products/9585874336) ₩38,400. *S형 로드셀 50kg* |
| 4 | **Eye bolts, M8, stainless** | [9220346388](https://www.coupang.com/vp/products/9220346388) | ~₩900 each | Select the M8 option; one each end of each cell. *스텐 아이볼트 M8* |
| 6 | **Locking carabiners, SUS304** | [8159611966](https://www.coupang.com/vp/products/8159611966) | ₩8,440 / 6 | Screw-lock: anchor ↔ cell ↔ band. *스텐 잠금비너* |
| 1 | **Door anchor kit (Melkin Sports)** | [5454522](https://www.coupang.com/vp/products/5454522) | ₩6,900 | Comes with a full band set plus handles, carabiner, ankle strap and door anchor. *멜킨스포츠 튜빙 도어앵커* |
| 2 | **Swim stretch cords with paddles** | [8916139496](https://www.coupang.com/vp/products/8916139496) (20 lb) / [8373410819](https://www.coupang.com/vp/products/8373410819) (30 lb) | ₩22,300 / ₩18,300 | One set has two cords; buy one per resistance level. *수영 스트레치코드* |
| 1 | **LiPo 3.7 V 1000 mAh** | [8811088847](https://www.coupang.com/vp/products/8811088847) | ₩14,900 | 503450, JST-PH 2.0 lead; solder to the XIAO battery pads, observe polarity |
| 1 | **Bench: 3-in-1 soft plyo box 40×50×60 cm** | [8785454056](https://www.coupang.com/vp/products/8785454056) | ₩40,250 | See [02-bench.md](02-bench.md) |
| 1 | Small enclosure | — | — | 3D-printed or a project box, hung at the door |
| — | Shielded 4-core cable (~1 m ×2), pin headers, heat-shrink | — | — | Load cell → HX711, if the cell's own lead is too short |
| — | USB-C **data** cable | — | — | Charge-only cables are the #1 "Mac can't see board" cause |

**Electronics total ≈ ₩140k** (dominated by the two S-type load cells), plus
≈ ₩80k for bench, cords and anchor kit.

**Cheaper/faster board alternative:** ESP32-S3 Super Mini,
[9458962970](https://www.coupang.com/vp/products/9458962970), ₩13,500, next-day
delivery. It works, but it has no battery charger and different pin labels:
use GPIO numbers in `config.h` (GPIO1–4 match the wiring below) and run it from
a USB power bank.

Bands: you'll calibrate each cord anyway, so brand doesn't matter. Prefer
latex tubing with a protective sleeve and paddle-style handles.

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
