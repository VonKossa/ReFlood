#!/usr/bin/env python3
"""Compose the exact Milestone-88 airborne-death snapshots."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

if len(sys.argv) != 6:
    raise SystemExit("usage: make_death_fall_preview.py OUT.png FRAME0.ppm ... FRAME3.ppm")
out_path=Path(sys.argv[1]);frames=[Image.open(p).convert("RGB") for p in sys.argv[2:]]
regular=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",13)
bold=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",18)
canvas=Image.new("RGB",(684,548),(9,12,17));draw=ImageDraw.Draw(canvas)
draw.text((20,14),"FLOOD MILESTONE 88 / AIRBORNE DEATH FALL",font=bold,fill=(238,241,245))
draw.text((20,43),"Right + Up stays held in every frame; $DBE8 clears wall contact and gravity takes over",font=regular,fill=(154,165,180))
labels=("death begins against wall","automatic fall","still falling with input held","south support: cross sequence")
for n,(frame,label) in enumerate(zip(frames,labels)):
    x=16+(n%2)*336;y=78+(n//2)*230
    crop=frame.crop((32,0,288,176)).resize((320,176),Image.Resampling.NEAREST)
    canvas.paste(crop,(x,y));draw.rectangle((x-1,y-1,x+320,y+176),outline=(64,73,88))
    draw.text((x,y+184),label,font=regular,fill=(211,217,226))
canvas.save(out_path)
