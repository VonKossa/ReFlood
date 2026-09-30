# Flood Milestone 61: exact status-strip reconstruction

## Result

Milestone 61 replaces the host's missing gameplay status display with a
translation of the resident 68000 builder at `$14388-$14552`, backed by the
original one-bit template and glyph bytes. The related compositor is at
`$14554-$145F8` and the gameplay loop calls it at `$AF7E`.

This work also falsifies the tentative assumption that the strip contains a
selected-weapon indicator. The gameplay selection word at `$E1EE` is not read
by the status builder or compositor.

## Recovered layout

| Region | Source | Transformation | Byte columns | Colour |
| --- | --- | --- | ---: | ---: |
| Air | word `$E1D2` | unsigned value `>> 1`, four fill segments | 0-3 | 13 |
| Score | longword `$17E8C` | five leading-zero digits | 11-15 | 15 |
| Remaining items | word `$17E72` | two leading-zero digits | 23-24 | 15 |
| Lives | word `$17E76` | two leading-zero digits | 33-34 | 15 |
| Life Force | word `$E1D0` | unsigned value `>> 4`, four fill segments | 36-39 | 5 |

The untouched columns come from the original 320-byte template at `$1423E`.
The glyph source starts at `$149FA`; each glyph is eight consecutive bytes,
one byte per row. Numeric glyph indices are the ASCII digit minus `$14`.

## Builder sequence

1. Copy the 320-byte template into the working mask.
2. Convert score, remaining items, and lives into the ten-byte numeric buffer
   as five, two, and two fixed-width digits plus the final terminator.
3. Stamp the numeric glyphs at byte columns 11, 23, and 33.
4. If Life Force is nonnegative, clear columns 36-39 and rebuild them from
   glyphs `$67-$6F`.
5. If air is nonnegative, clear columns 0-3 and rebuild them in the same way.

Each gauge decomposes its shifted value into four successive 0-8 pixel
amounts. The routine processes thresholds 24, 16, 8, and 0 while writing from
the inside column toward the outside edge.

## Compositor and host boundary

The original compositor divides the mask into three colour paths: the first
four bytes use palette entry 13, the central 32 bytes use entry 15, and the
last four bytes use entry 5. It stamps the eight scanlines after gameplay
rendering at display Y=16.

The original X input is derived from a packed scroll-phase value. The compact
352-pixel SDL viewport currently centres the 320-pixel strip at X=16. This is
an explicit presentation adaptation, not a claim that the original packed X
calculation has been reconstructed.

## Extracted assets

`tools/extract_hud_assets.py` mechanically maps resident addresses from the
recovered ADF and emits:

| File | Size | SHA-256 |
| --- | ---: | --- |
| `data/hud/HUD_template.bin` | 320 | `2afbb6fc0891911c0d4d8adc4015493b46caa1d6d24d149dd63b5a2112f619ff` |
| `data/hud/HUD_glyphs.bin` | 896 | `151749d8ba1102e31de6ac7d8ebf12b4d62ba74fa4a5046b83c7f90d40a5e691` |

## Verification

The core regression test fixes score to 12345, items to 07, lives to 03,
Life Force to 511, and air to 63. It checks the resulting numeric buffer,
selected glyph rows, and all gauge columns. A second case proves that negative
counter values clamp to zero while negative Life Force and air preserve the
template, matching the builder's signed branches.

The deliverable is compiled with strict C99 warnings as errors, exercised by
the complete 42-level headless load/run sweep, and repeated under undefined-
behaviour sanitization.

## Next accuracy target

Milestone 62 should recover the packed camera/scroll-phase value passed to the
status compositor and reproduce its exact horizontal placement and clipping
relative to the original playfield fetch window.
