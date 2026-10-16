#!/usr/bin/env python3
"""Build a band calibration table (force vs stretch) for firmware/src/bands.h.

Input: a CSV of steady holds, one row per hold, with columns
    stretch_cm,force_n
Either type them by hand, or capture the firmware's `stream on` output and
use --from-stream with a list of hold windows (see below).

Usage
    python3 tools/calibrate_band.py holds.csv --name LVL2_L
    python3 tools/calibrate_band.py stream.csv --from-stream L \
        --holds "0:2000-4000,10:6000-8000,20:10000-12000"

--holds maps stretch_cm to a time window (ms) in the stream; the median force
of each window is used. --from-stream picks the L or R column.

Output: a C++ BandPoint array ready to paste into bands.h. Points are sorted,
the first point is forced to (0 m, 0 N) if missing, and non-monotonic points
(measurement noise) are dropped with a warning.
"""

import argparse
import csv
import statistics
import sys


def read_holds(path):
    points = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            points.append((float(row["stretch_cm"]) / 100.0, float(row["force_n"])))
    return points


def read_stream(path, side, holds_spec):
    col = "F_L" if side.upper() == "L" else "F_R"
    samples = []
    with open(path, newline="") as f:
        reader = csv.DictReader(line for line in f if line[:1].isdigit() or line.startswith("t_ms"))
        for row in reader:
            try:
                samples.append((int(row["t_ms"]), float(row[col])))
            except (KeyError, ValueError):
                continue
    points = []
    for item in holds_spec.split(","):
        cm, window = item.split(":")
        start, end = (int(v) for v in window.split("-"))
        forces = [fv for t, fv in samples if start <= t <= end]
        if not forces:
            sys.exit(f"no samples in window {window} for {cm} cm")
        points.append((float(cm) / 100.0, statistics.median(forces)))
    return points


def clean(points):
    points = sorted(points)
    if not points or points[0][0] > 1e-6:
        points.insert(0, (0.0, 0.0))
    out = [points[0]]
    for x, f in points[1:]:
        if x > out[-1][0] and f > out[-1][1]:
            out.append((x, f))
        else:
            print(f"// warning: dropped non-monotonic point {x * 100:.0f} cm / {f:.1f} N",
                  file=sys.stderr)
    if len(out) > 16:
        sys.exit("firmware supports at most 16 points; use coarser stretch steps")
    return out


def energy(points):
    e = 0.0
    for (x0, f0), (x1, f1) in zip(points, points[1:]):
        e += 0.5 * (f0 + f1) * (x1 - x0)
    return e


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--name", default="kMyBand")
    ap.add_argument("--from-stream", metavar="L|R")
    ap.add_argument("--holds", help="cm:start-end,... windows in ms (with --from-stream)")
    args = ap.parse_args()

    if args.from_stream:
        if not args.holds:
            sys.exit("--from-stream needs --holds")
        pts = read_stream(args.csv, args.from_stream, args.holds)
    else:
        pts = read_holds(args.csv)
    pts = clean(pts)

    name = args.name if args.name.startswith("k") else "k" + args.name
    print(f"// {len(pts)} points, energy to {pts[-1][0] * 100:.0f} cm = {energy(pts):.1f} J")
    print(f"constexpr BandPoint {name}[] = {{")
    body = ", ".join(f"{{{x:.3f}f, {f:.1f}f}}" for x, f in pts)
    print(f"    {body},")
    print("};")


if __name__ == "__main__":
    main()
