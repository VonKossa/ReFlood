# Flood Milestone 38 — plane order and Quiffy contact ring

## Disk mapping

The supplied recovered ADF is a custom-loader disk rather than an AmigaDOS
filesystem. Track/cylinder data beginning at byte 11,264 is the resident image
loaded at `$9530`. Thus runtime address `A` is found at disk offset
`11264 + A - $9530`. As a cross-check, `$B60E` maps to byte 19,678 and contains
the known 32-word palette table exactly.

## Palette / bitplanes

The 32 palette words at `$B60E` are genuine. The wrong Milestone 37 appearance
was caused by treating asset plane blocks as ascending color significance.
Original blitter/display ordering maps file planes to color bits `1,0,3,2`.
Milestone 38 applies this mapping to both BLOCK tiles and sprite image planes.
It corresponds to permutation 07 in the supplied diagnostic sheet and matches
captured original Amiga terrain: orange/brown rock, blue recesses and green
trim. No palette-bank substitution is needed.

## Exact eight-direction contacts

`flood_build_quiffy_contacts()` reconstructs `$F22C-$F4B0`. Four one-pixel
diagonal `$F0E0` probes surround Quiffy's centered 16x16 collision core at
sprite offset `(+8,+8)`. They build the ring:

`bit0 N, bit1 NE, bit2 E, bit3 SE, bit4 S, bit5 SW, bit6 W, bit7 NW`.

Terrain attribute bit 6 raises the original auxiliary bit 8. Attribute bit 7
raises auxiliary bit 9 and triggers the original straight-down confirmation;
confirmed support becomes `S|SE|SW`. The public player state retains the low
eight contact bits and their population count.

`flood_resolve_quiffy_contacts()` reconstructs `$D99E-$DAEA`, including the
original S, N, E, W priority and corner-direction decisions. Attachment mode 1
is horizontal floor/ceiling traversal; mode 2 is vertical wall traversal.

## Verification

The headless core builds cleanly with strict C99 warnings. Tests cover swept
collision, full south and east contact triplets, contact count, attachment
mode, real level loading, and corrected Quiffy sprite selection. All pass.

## Remaining accuracy boundary

The contact sensing and attachment decision are binary-derived. Higher-level
joystick acceleration, corner interpolation, sprite orientation and all
animation transitions surrounding those routines still need reconstruction.
