# Flood Milestone 63: exact camera and visible window

## Result

Milestone 63 replaces the SDL host's approximate `player-(176,120)` camera and
352x240 presentation with the original camera formula, level bounds, and
320x208 visible window. It also fixes a reconstruction bug that left camera
bounds at zero when a standalone level contained no marker 23.

## Camera update

The complete camera update is at `$D94A-$D990`:

```text
camera_x = clamp_signed(player_x - 152, 0, max_x)
camera_y = clamp_signed(player_y -  92, 0, max_y)
```

The source player words are `$180AC/$180AE`; the destination camera words are
`$17E82/$17E84`. Each axis follows the same exact branch order: subtract the
anchor, replace nonpositive results with zero, and replace values greater than
or equal to the cavern maximum with that maximum. There is no interpolation,
velocity, or easing in this routine.

The host stores these words as `camera_x/camera_y` and updates them after
Quiffy's movement on every gameplay tick. Rendering now uses
`screen=world-camera` directly.

## Default and marker-derived bounds

Before scanning a new map, `$E726-$E732` installs:

| Axis | Default | Full-map derivation |
| --- | ---: | --- |
| X maximum `$E6BA` | `$06C0` = 1728 | `2048 - 320` |
| Y maximum `$E6B8` | `$0570` = 1392 | `1600 - 208` |

Marker 23 at `$EB4A-$EB7C` replaces those values with:

```text
max_x = marker_tile_x * 16 - 320
max_y = max(marker_tile_y * 16 - 208, 0)
```

The original code explicitly clamps the marker-derived Y maximum but does not
repeat that clamp for X. The reconstruction preserves this asymmetry.

Milestone 45 already translated marker 23, but the full-map defaults were
missing. Because most shipped levels contain no marker 23, standalone loads
then retained the zero-filled host values. Milestone 63 initializes the exact
defaults before marker processing. Regression cases verify level 1 defaults,
level 5's `(1040,432)` marker bounds, and level 8's `(0,0)` bounds.

## Why the visible window is 320x208

The old host inferred 352x240 from the backing allocation: four planes are
separated by `$2940`, and `$2940 / 44 = 240` rows. That describes storage, not
the Copper display window.

The resident evidence is mutually consistent:

- `$EE9A/$EEA0` installs `DIWSTRT=$2C81` and `DIWSTOP=$FCC1`, exposing 208
  vertical lines.
- `$EE8E-$EE94` installs a 44-byte row stride through modulo 2 and the
  horizontal-scroll fetch setup.
- `$EC78/$ECAE` runs 14 tile rows and `$ECB2` runs 22 tile columns: one extra
  16-pixel tile on both axes for fine scrolling.
- Default camera maxima subtract exactly 320 and 208 from the 2048x1600 map.

The *Amiga Hardware Reference Manual* explains that smooth horizontal
scrolling starts data fetch one word early and fetches two extra bytes without
enlarging the display window. See [Specifying Data Fetch in Horizontal
Scrolling](https://www.amigascene.nl/AmigaDevDocs/Hardware_Manual/Hardware_Manual_guide/node0089.html)
and [Specifying the Modulo in Horizontal Scrolling](https://www.amigascene.nl/AmigaDevDocs/Hardware_Manual/Hardware_Manual_guide/node008A.html).

The SDL logical window is therefore now 320x208. The exact HUD covers visible
X=0-319 at Y=16-23, while the recovered camera makes a freely tracked Quiffy
appear at sprite coordinate `(152,92)`.

## Related runtime correction

The State-11/16-18 mechanism visibility gate previously recomputed the host's
approximate camera locally. It now consumes the recovered camera state, keeping
camera-dependent mechanism activation aligned with the renderer.

## Verification

Regression coverage checks:

- zero, free-follow, and maximum-clamp cases on both axes;
- exact camera anchor `(152,92)`;
- full-map and marker-derived bounds;
- all 16 HUD fine-scroll phases from Milestone 62;
- all existing gameplay reconstruction tests.

The complete 42-level standalone load/run sweep is repeated with strict C99
warnings and undefined-behaviour sanitization.

## Next accuracy target

Milestone 64 should reconstruct the map-tile blitter's exact 22x14 overfetch,
including coarse tile selection, X/Y fine phases, and edge padding. That will
replace the SDL renderer's direct per-pixel sampling with a testable model of
the original backing-buffer composition without changing the now-correct
320x208 visible output.
