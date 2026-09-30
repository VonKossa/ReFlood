# Flood Milestone 62: exact HUD scroll-phase placement

## Result

Milestone 62 closes the horizontal-placement boundary left by Milestone 61.
The original status strip is screen-fixed: it covers visible X=0-319 at
Y=16-23 for every camera fine-scroll phase. The apparent X=16 in the host is
the visible-window origin inside the host's historical 44-byte-row view.

## Resident-code trace

The gameplay loop copies camera X from `$17E82` to local `-$0C(A5)`. At
`$A87E-$A88A` it masks that value with `$000F`, stores the result at
`-$0A(A5)`, and preserves the same phase at `-$10(A5)`. The compositor call at
`$AF76-$AF84` pushes Y=16 and this preserved phase before calling
`$14554`.

The phase also controls two other parts of the display setup:

- Phase zero sets `$17E54` to the current frame pointer minus `$2BE`.
- Phases 1-15 set `$17E54` to the frame pointer minus `$2C0`.
- `$A8F0-$A904` computes `(16-phase)&15`, duplicates the nibble, and later
  writes it to the Copper list's `BPLCON1` data word at offset `$0E`.

The duplicated values run `$00,$FF,$EE,...,$11` for camera phases 0-15,
setting equal fine-scroll delay for both playfields.

## Compositor geometry

`$1455E-$14576` computes:

```text
row offset    = Y * 44
coarse offset = 2 * floor(X / 16)
bit shift     = X & 15
```

The gameplay caller supplies X in 0-15, so its coarse offset is always zero.
At Y=16 the row offset is 704 (`$2C0`) bytes. Relative to the current frame
pointer, the resulting raw mask position is therefore:

| Camera phase | `$17E54` base | Position after Y offset | Mask bit shift | Backing X |
| ---: | ---: | ---: | ---: | ---: |
| 0 | `frame-$2BE` | `frame+2` | 0 | 16 |
| 1-15 | `frame-$2C0` | `frame+0` | phase | phase |

The complementary `BPLCON1` delay completes the invariant:

```text
phase 0:     backing X 16 + delay 0          = fetch X 16
phase 1-15: backing X p  + delay (16 - p)   = fetch X 16
```

The 320-pixel mask consequently ends at fetch X=335. The 320-pixel display
window begins after the extra horizontal-scroll fetch word, mapping fetch
X=16-335 to visible X=0-319.

This interpretation agrees with the *Amiga Hardware Reference Manual*: a
horizontally scrolled playfield starts data fetch one word early with
`DDFSTRT=$0030`, fetches two extra bytes per line, and uses `BPLCON1` for a
0-15-bit delay. Flood's Copper builder at `$EE4A` installs exactly that
`DDFSTRT`, `DDFSTOP=$00D0`, and a modulo of 2. See the manual sections
[Specifying Data Fetch in Horizontal Scrolling](https://www.amigascene.nl/AmigaDevDocs/Hardware_Manual/Hardware_Manual_guide/node0089.html)
and [Specifying the Modulo in Horizontal Scrolling](https://www.amigascene.nl/AmigaDevDocs/Hardware_Manual/Hardware_Manual_guide/node008A.html).

## SDL representation

The compact SDL host has historically exposed 352 pixels, corresponding to
the 44-byte memory-row stride rather than the original 320-pixel display
window. Changing that viewport would alter the camera and every existing host
composition path, so Milestone 62 does not disguise it as a HUD-only change.
Instead, `flood_hud_placement()` models the recovered hardware geometry and
maps original visible X=0 to host X=16 explicitly. The renderer now obtains
its HUD origin from that model.

## Verification

The new regression iterates all 16 camera phases and checks:

- camera phase and complementary delay;
- duplicated `BPLCON1` byte `$00,$FF,...,$11`;
- the phase-dependent `$2BE/$2C0` base bias;
- frame-relative byte offset and backing bit position;
- invariant fetch X=16 and visible X=0;
- exact right edges at fetch X=336 and visible X=320.

The full core regression and 42-level headless sweep are also run under strict
C99 warnings and undefined-behaviour sanitization.

## Next accuracy target

Milestone 63 should reconstruct the camera-follow update that produces
`$17E82/$17E84`, then decide the larger renderer correction as one coherent
change: exact 320-pixel display-window cropping, camera bounds, and world-to-
screen conversion rather than an isolated viewport-size substitution.
