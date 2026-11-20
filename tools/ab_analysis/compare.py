#!/usr/bin/env python3
"""Compare time-aligned PCM16 WAV renders, without changing the audio path.

No third-party Python packages required. Offline measurements only; playback/
rendering through the VST2 and VST3 plugins must happen in a separate host.
"""
import argparse
import csv
import json
import math
import struct
import wave
from pathlib import Path

EPS = 1e-12

def load_wav(path):
    with wave.open(str(path), "rb") as w:
        if w.getcomptype() != "NONE" or w.getsampwidth() != 2:
            raise ValueError(f"{path}: use uncompressed 16-bit PCM WAV")
        channels, rate, frames = w.getnchannels(), w.getframerate(), w.getnframes()
        if channels not in (1, 2):
            raise ValueError(f"{path}: only mono or stereo supported")
        raw = w.readframes(frames)
    values = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    return {"path": str(path), "channels": channels, "rate": rate,
            "frames": frames, "values": values}

def rms_db(samples):
    if not samples:
        return -160.0
    rms = math.sqrt(sum(x * x for x in samples) / len(samples))
    return 20 * math.log10(max(EPS, rms))

def peak_db(samples):
    return 20 * math.log10(max(EPS, max((abs(v) for v in samples), default=0)))

def db_of_pcm(values):
    return rms_db([v / 32768.0 for v in values])

def measure_window(file, start, end):
    ch = file["channels"]
    values = file["values"][start * ch: end * ch]
    mono = [(sum(values[i:i + ch]) / ch) / 32768.0
            for i in range(0, len(values), ch)]
    return rms_db(mono), peak_db([v / 32768.0 for v in values])

def percentile(values, pct):
    if not values:
        return None
    ordered = sorted(values)
    i = (len(ordered) - 1) * pct / 100
    lo = int(math.floor(i))
    hi = int(math.ceil(i))
    if lo == hi:
        return ordered[lo]
    return ordered[lo] * (hi - i) + ordered[hi] * (i - lo)

def summarize(rows, name):
    levels = [r[name + "_rms_dbfs"] for r in rows]
    gains = [r[name + "_gain_db"] for r in rows
             if r["input_rms_dbfs"] > -90 and r[name + "_rms_dbfs"] > -90]
    quiet = [r[name + "_gain_db"] for r in rows
             if -70 <= r["input_rms_dbfs"] < -40
             and r[name + "_rms_dbfs"] > -90]
    loud = [r[name + "_gain_db"] for r in rows
            if r["input_rms_dbfs"] >= -25
            and r[name + "_rms_dbfs"] > -90]
    return {"median_level_dbfs": percentile(levels, 50),
            "p95_level_dbfs": percentile(levels, 95),
            "median_gain_db": percentile(gains, 50),
            "median_quiet_gain_db": percentile(quiet, 50),
            "median_loud_gain_db": percentile(loud, 50),
            "peak_dbfs": max(r[name + "_peak_dbfs"] for r in rows)}

def compare(original, outlaw, smoothride, window_ms=100):
    files = [load_wav(p) for p in (original, outlaw, smoothride)]
    if len({(f["rate"], f["channels"]) for f in files}) != 1:
        raise ValueError("All three WAVs must have identical sample rate and channel count")
    frame_counts = [f["frames"] for f in files]
    if max(frame_counts) - min(frame_counts) > 1:
        raise ValueError("Files have different lengths. Export aligned, equally long renders.")
    count = min(frame_counts)
    rate = files[0]["rate"]
    step = max(1, round(rate * window_ms / 1000))
    keys = ("input", "outlaw", "smoothride")
    rows = []
    for start in range(0, count, step):
        end = min(count, start + step)
        row = {"time_s": round(start / rate, 6), "duration_s": (end-start)/rate}
        for key, file in zip(keys, files):
            level, peak = measure_window(file, start, end)
            row[key + "_rms_dbfs"] = round(level, 4)
            row[key + "_peak_dbfs"] = round(peak, 4)
        for key in ("outlaw", "smoothride"):
            row[key + "_gain_db"] = round(
                row[key + "_rms_dbfs"] - row["input_rms_dbfs"], 4)
        rows.append(row)
    if not rows:
        raise ValueError("Input WAV files contain no audio frames")
    return rows, {"format": "PCM16", "sample_rate": rate,
                  "channels": files[0]["channels"], "frames_compared": count,
                  "duration_s": count / rate, "window_ms": window_ms,
                  "outlaw": summarize(rows, "outlaw"),
                  "smoothride": summarize(rows, "smoothride"),
                  "caveats": [
                      "Window RMS is not integrated LUFS or perceived loudness.",
                      "Gain estimates assume aligned exports and no other processing.",
                      "Windows below -90 dBFS are excluded from gain summaries.",
                      "Quiet/loud bands use input RMS: [-70,-40) / >=-25 dBFS.",
                      "Compare matched output loudness before judging ambience."
                  ]}

def save_svg(path, rows):
    w, h, left, top, chartw, charth = 980, 390, 66, 30, 865, 290
    def coords(key):
        points = []
        for i, row in enumerate(rows):
            x = left + (i / max(1, len(rows)-1)) * chartw
            y = top + (8 - max(-24, min(8, row[key]))) / 32 * charth
            points.append(f"{x:.1f},{y:.1f}")
        return " ".join(points)
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
             '<rect width="100%" height="100%" fill="#141a20"/>',
             '<text x="65" y="22" font-size="16" fill="#e9edf1">Windowed gain vs input (dB)</text>']
    for db in (-24,-16,-8,0,8):
        y=top + (8-db)/32*charth
        parts.append(f'<line x1="{left}" y1="{y:.1f}" x2="{left+chartw}" y2="{y:.1f}" stroke="#35414b"/>')
        parts.append(f'<text x="12" y="{y+4:.1f}" font-size="12" fill="#b8c0c8">{db:+} dB</text>')
    for key,color in (("outlaw_gain_db","#ffbc66"),("smoothride_gain_db","#48d3c5")):
        parts.append(f'<polyline points="{coords(key)}" fill="none" stroke="{color}" stroke-width="1.7"/>')
    parts.extend(['<text x="65" y="355" fill="#ffbc66">OutlawCM</text>',
                  '<text x="185" y="355" fill="#48d3c5">SmoothRide</text>',
                  '<text x="400" y="355" fill="#aab5bd">Time → (100 ms windows by default)</text>', '</svg>'])
    path.write_text("\n".join(parts), encoding="utf-8")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", type=Path, help="Original dry 16-bit PCM WAV")
    p.add_argument("outlaw", type=Path, help="OutlawCM VST2 WAV render")
    p.add_argument("smoothride", type=Path, help="SmoothRide VST3 WAV render")
    p.add_argument("--out", type=Path, default=Path("ab-results"))
    p.add_argument("--window-ms", type=int, default=100)
    args=p.parse_args()
    if not (10 <= args.window_ms <= 1000):
        p.error("--window-ms must be between 10 and 1000")
    rows, summary=compare(args.input,args.outlaw,args.smoothride,args.window_ms)
    args.out.mkdir(parents=True, exist_ok=True)
    with (args.out / "windows.csv").open("w",newline="",encoding="utf-8") as fp:
        writer=csv.DictWriter(fp, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (args.out / "summary.json").write_text(json.dumps(summary,indent=2),encoding="utf-8")
    save_svg(args.out / "gain_comparison.svg",rows)
    print(json.dumps(summary,indent=2))
    print(f"Results: {args.out.resolve()}")

if __name__=="__main__":
    main()
