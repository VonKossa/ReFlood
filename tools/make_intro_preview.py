#!/usr/bin/env python3
"""Compose centred PAL-canvas intro frames into the Milestone-87 preview."""
import sys
from pathlib import Path
from PIL import Image,ImageDraw

if len(sys.argv)!=8:raise SystemExit("usage: make_intro_preview.py OUT.png FRAME0.ppm ... FRAME5.ppm")
target=Path(sys.argv[1]);frames=[Image.open(p).convert("RGB") for p in sys.argv[2:]]
labels=("frame 0 - loaded image","frame 1 - opening blits","frame 240","frame 320","frame 512","frame 1215 - final view")
panel_w,panel_h=320,270;out=Image.new("RGB",(1004,630),(9,12,17));draw=ImageDraw.Draw(out)
draw.text((20,15),"FLOOD MILESTONE 87 / CENTRED BULLFROG INTRO",fill=(238,241,245))
draw.text((20,36),"Native 160x96 animation at (80,72) inside the 320x240 PAL presentation canvas",fill=(150,163,181))
for n,(frame,label) in enumerate(zip(frames,labels)):
    x=10+(n%3)*331;y=70+(n//3)*270
    out.paste(frame,(x,y));draw.rectangle((x-1,y-1,x+320,y+240),outline=(64,73,88));draw.text((x+4,y+245),label,fill=(211,217,226))
draw.text((20,612),"8.354 s music lead + 1216 steps (24.32 s at 20 ms)",fill=(150,163,181))
out.save(target)
