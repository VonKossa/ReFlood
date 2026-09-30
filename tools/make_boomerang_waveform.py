#!/usr/bin/env python3
"""Draw the two exact notes in boomerang sound 24."""
import struct
import sys
import wave
from pathlib import Path
from PIL import Image, ImageDraw

source, target = map(Path, sys.argv[1:3])
with wave.open(str(source), "rb") as wav:
    rate, frames = wav.getframerate(), wav.getnframes()
    samples = struct.unpack("<" + "h" * frames * 2, wav.readframes(frames))

width, height = 1200, 360
image = Image.new("RGB", (width, height), (10, 13, 18))
draw = ImageDraw.Draw(image)
draw.text((24, 16), "FLOOD MILESTONE 87 / BOOMERANG SOUND 24", fill=(238, 241, 245))
draw.text((24, 39), "exact shared envelope: C7 ramps first note 1→63; C8 fades second note 63→0", fill=(154, 165, 180))
mid = 190
draw.line((20, mid, width - 20, mid), fill=(55, 63, 75))
for x in range(20, width - 20):
    first = (x - 20) * frames // (width - 40)
    span = max(1, frames // (width - 40))
    last = min(frames, first + span)
    peak = max(abs(samples[n * 2 + 1]) for n in range(first, last))
    extent = peak * 105 // 32768
    draw.line((x, mid - extent, x, mid + extent), fill=(89, 195, 255))
split = 20 + int(1.80 / 4.0 * (width - 40))
draw.line((split, 75, split, 300), fill=(225, 178, 92))
draw.text((110, 92), "first note / period 559", fill=(211, 217, 226))
draw.text((110, 113), "was held at inaudible volume 1", fill=(150, 163, 181))
draw.text((split + 24, 92), "second note / period 370", fill=(211, 217, 226))
for second in range(5):
    x = 20 + second * (width - 40) // 4
    draw.line((x, 305, x, 313), fill=(90, 101, 117))
    draw.text((x - 4, 320), f"{second}s", fill=(150, 163, 181))
image.save(target)
