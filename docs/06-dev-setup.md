# Development setup (macOS) and calibration workflow

## What an ESP32 is

A low-cost 32-bit microcontroller from Espressif with Wi-Fi and Bluetooth LE.
It runs one program (the firmware) forever. We use the **Arduino framework**
on it (Arduino libraries and `setup()/loop()`), built with **PlatformIO**. It is
not an Arduino board, but it runs Arduino-style code.

## Toolchain

1. Install VS Code and the **PlatformIO IDE** extension (or the CLI:
   `brew install platformio`).
2. Open the `firmware/` folder. PlatformIO downloads the ESP32 toolchain and
   libraries on first build.
3. Plug the XIAO ESP32-S3 in with a **data** USB-C cable. No driver needed.
4. `pio run -t upload` then `pio device monitor` (115200 baud).

If upload fails: hold **BOOT**, tap **RESET**, release BOOT, upload again.

## Host tests (no hardware needed)

The power math lives in `firmware/lib/swimpower` with no Arduino dependencies:

```bash
cd firmware
pio test -e native
```

## Serial commands

| Command | Action |
|---|---|
| `tare` | Zero both load cells with no load |
| `cal L <kg>` / `cal R <kg>` | Calibrate a load cell with a known hanging mass |
| `stream on` / `stream off` | CSV stream: `t_ms,F_L,F_R` (N, filtered) |
| `rate` | Print measured samples/s per side |
| `band 1` / `band 2` / `band 3` | Select the band calibration set (LVL1–3) |
| `thr <on> <off>` | Set stroke detection thresholds in newtons above baseline |
| `status` | Print calibration, thresholds, forces, power, stroke counts |
| `save` | Store load-cell calibration and band choice in flash |

When not streaming, every detected stroke is printed:
`stroke L  peak 84 N  work 21.3 J  dur 710 ms  rise 330 ms`.

## Step 1: calibrate the load cells

1. Hang each cell from the anchor with nothing attached → `tare`.
2. Hang a known mass, e.g. a 5 L water bottle weighed on a kitchen scale →
   `cal L 5.0` (then same for R) → `save`.

## Step 2: calibrate each band (force vs stretch)

1. Clip the band to the load cell. Put tape marks on the floor from the point
   where the band just goes taut: 0, 10, 20, … cm.
2. `stream on`. Pull the paddle slowly to each mark and hold ~2 s.
3. Copy the serial output to a CSV, note the stretch for each hold, and run:

   ```bash
   python3 tools/calibrate_band.py holds.csv --name LVL2_L
   ```

   It prints a C++ table to paste into `firmware/src/bands.h`.

## Step 3: tune stroke detection

`stream on` during a real set, save the CSV, and check that each pull crosses
`on_threshold` and drops below `off_threshold` between strokes. Adjust in
`config.h`.

## Working with Claude

Docs and code skeleton were written in a cloud session. Hardware work (flash,
pull, read the serial output, adjust) goes best in **Claude Code on the Mac**
with this repo cloned, since it can reach the USB port.
