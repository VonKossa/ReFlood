#!/usr/bin/env python3
"""Show the corrected settled-water oxygen progression in the exact HUD."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from make_hud_preview import build

root = Path(__file__).resolve().parents[1]
regular = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
bold = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)
canvas = Image.new("RGB", (1040, 300), (10, 13, 18))
draw = ImageDraw.Draw(canvas)
draw.text((24, 18), "FLOOD MILESTONE 87 / SETTLED-WATER OXYGEN", font=bold, fill=(238, 241, 245))
draw.text((24, 47), "$109E6[40] = 16; $DECC consumes air on alternating gameplay frames", font=regular, fill=(154, 165, 180))
for panel, (air, ticks) in enumerate(((63, 0), (61, 4), (59, 8))):
    x = 24 + panel * 336
    hud, _ = build(0, 7, 3, 511, air)
    canvas.paste(hud.resize((320, 32), Image.Resampling.NEAREST), (x, 108))
    draw.rectangle((x - 1, 107, x + 320, 140), outline=(64, 73, 88))
    draw.text((x, 157), f"air = {air} after {ticks} ticks", font=regular, fill=(211, 217, 226))
draw.text((24, 220), "Previous table: state 40 → depth 0 → oxygen restored", font=regular, fill=(224, 130, 118))
draw.text((24, 248), "Correct table: state 40 → depth 16 → oxygen decreases, then drowning damage", font=regular, fill=(116, 211, 166))
canvas.save(root / "oxygen_meter_milestone87.png")
