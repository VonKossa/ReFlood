#!/usr/bin/env python3
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "exact_hud_milestone61.png"
PAL12 = [
    0x000, 0x323, 0x548, 0x689, 0x610, 0x730, 0xB60, 0xC92,
    0x341, 0x451, 0x560, 0x670, 0x236, 0x367, 0x89A, 0xCB9,
]
PAL = [tuple(((v >> shift) & 15) * 17 for shift in (8, 4, 0)) for v in PAL12]
REGULAR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FONT = ImageFont.truetype(REGULAR, 16)
SMALL = ImageFont.truetype(REGULAR, 13)
HEAD = ImageFont.truetype(BOLD, 18)
TITLE = ImageFont.truetype(BOLD, 28)


def fixed(value, digits):
    return f"{max(value, 0):0{digits}d}"[-digits:]


def copy_glyph(mask, glyphs, glyph, column):
    for y in range(8):
        mask[y * 40 + column] = glyphs[glyph * 8 + y]


def gauge(mask, glyphs, value, first):
    for y in range(8):
        mask[y * 40 + first:y * 40 + first + 4] = b"\0\0\0\0"
    remaining = value
    for segment in range(4):
        threshold = 24 - segment * 8
        part = max(remaining - threshold, 0)
        remaining -= part
        copy_glyph(mask, glyphs, 0x67 + part, first + 3 - segment)


def build(score, items, lives, life, air):
    mask = bytearray((ROOT / "data/hud/HUD_template.bin").read_bytes())
    glyphs = (ROOT / "data/hud/HUD_glyphs.bin").read_bytes()
    digits = fixed(score, 5) + fixed(items, 2) + fixed(lives, 2)
    for n in range(5): copy_glyph(mask, glyphs, ord(digits[n]) - 0x14, 11 + n)
    for n in range(2): copy_glyph(mask, glyphs, ord(digits[5 + n]) - 0x14, 23 + n)
    for n in range(2): copy_glyph(mask, glyphs, ord(digits[7 + n]) - 0x14, 33 + n)
    if life >= 0: gauge(mask, glyphs, life >> 4, 36)
    if air >= 0: gauge(mask, glyphs, air >> 1, 0)
    image = Image.new("RGB", (320, 8), PAL[0])
    px = image.load()
    for y in range(8):
        for bx in range(40):
            colour = PAL[13 if bx < 4 else 5 if bx >= 36 else 15]
            bits = mask[y * 40 + bx]
            for bit in range(8):
                if bits & (0x80 >> bit): px[bx * 8 + bit, y] = colour
    return image, digits


def main():
    hud, digits = build(12345, 7, 3, 511, 63)
    canvas = Image.new("RGB", (1320, 700), (20, 22, 28))
    draw = ImageDraw.Draw(canvas)
    draw.text((40, 25), "MILESTONE 61 / ORIGINAL STATUS STRIP", font=TITLE, fill=(245, 246, 248))
    draw.text((42, 66), "$14388-$145F8 reconstructed from the resident code and original one-bit artwork",
              font=FONT, fill=(159, 171, 191))

    draw.rounded_rectangle((40, 112, 1280, 270), 14, fill=(31, 34, 42), outline=(74, 89, 111), width=2)
    enlarged = hud.resize((1280, 32), Image.Resampling.NEAREST)
    canvas.paste(enlarged, (20, 169))
    draw.text((60, 128), "Actual 320x8 strip shown at 4x", font=HEAD, fill=(224, 229, 237))
    draw.text((60, 222), f"sample numeric buffer: {digits}", font=SMALL, fill=(173, 184, 201))

    columns = [
        (40, 378, "AIR", "$E1D2 >> 1", "4 x 8-pixel segments", PAL[13]),
        (304, 588, "SCORE", "$17E8C", "five fixed digits", PAL[15]),
        (614, 878, "ITEMS", "$17E72", "two fixed digits", PAL[15]),
        (904, 1088, "LIVES", "$17E76", "two-digit conversion", PAL[15]),
        (1114, 1280, "LIFE", "$E1D0 >> 4", "4 segments", PAL[5]),
    ]
    for x1, x2, label, address, detail, colour in columns:
        draw.rounded_rectangle((x1, 310, x2, 465), 12, fill=(37, 42, 51), outline=colour, width=2)
        draw.text((x1 + 16, 330), label, font=HEAD, fill=colour)
        draw.text((x1 + 16, 370), address, font=FONT, fill=(230, 232, 236))
        draw.text((x1 + 16, 408), detail, font=SMALL, fill=(165, 176, 192))

    draw.rounded_rectangle((40, 505, 1280, 656), 14, fill=(35, 48, 45), outline=(67, 128, 104), width=2)
    draw.text((62, 527), "EXACT BUILD ORDER", font=HEAD, fill=(158, 228, 195))
    steps = [
        (64, 569, "1  Copy the 320-byte template"),
        (64, 605, "2  Convert counters to 5 + 2 + 2 fixed digits"),
        (64, 631, "3  Stamp glyphs at byte columns 11, 23, and 33"),
        (670, 569, "4  Rebuild Life Force, then air"),
        (670, 605, "5  Composite last, at visible display Y=0"),
    ]
    for x, y, line in steps:
        draw.text((x, y), line, font=SMALL, fill=(214, 224, 219))
    canvas.save(OUT, optimize=True)
    print(OUT)


if __name__ == "__main__":
    main()
