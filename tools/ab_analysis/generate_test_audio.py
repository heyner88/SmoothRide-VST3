#!/usr/bin/env python3
"""Generate original synthetic stereo street-like test material (no downloads)."""
import argparse
import math
import random
import struct
import wave
from pathlib import Path

def render(out, rate=48000, seconds=24):
    rng=random.Random(20261120)
    frames=rate*seconds
    out=Path(out)
    out.parent.mkdir(parents=True,exist_ok=True)
    with wave.open(str(out),"wb") as file:
        file.setnchannels(2)
        file.setsampwidth(2)
        file.setframerate(rate)
        for i in range(frames):
            t=i/rate
            # Continuous low-level street ambience, plus passing traffic and
            # alternating synthetic 'foreground' bursts (not actual speech).
            traffic=0.017*(1+0.7*math.sin(2*math.pi*0.17*t))*math.sin(2*math.pi*83*t)
            ambience=0.008*rng.uniform(-1,1)
            foreground=(0.13*(0.5+0.5*math.sin(2*math.pi*4.2*t))
                        *math.sin(2*math.pi*220*t)
                        if 3<t<7 or 11<t<15 or 19<t<22 else 0)
            for_right=ambience*0.88+traffic*0.97+foreground*0.84
            for_left=ambience+traffic+foreground
            file.writeframesraw(struct.pack("<hh",
                int(max(-1,min(1,for_left))*32767),
                int(max(-1,min(1,for_right))*32767)))
    print(f"Created {out} ({seconds}s, {rate}Hz, stereo PCM16)")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--out", default="ab-test-input.wav")
    p.add_argument("--seconds",type=int,default=24)
    p.add_argument("--rate",type=int,default=48000)
    a=p.parse_args()
    if not (3<=a.seconds<=120): p.error("seconds must be 3..120")
    if a.rate not in (44100,48000,96000): p.error("rate must be 44100/48000/96000")
    render(a.out,a.rate,a.seconds)
