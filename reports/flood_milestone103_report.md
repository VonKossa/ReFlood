# Flood reconstruction — Milestone 103

## Front-end viewport and aspect-ratio audit

The title, PLAY LEVEL/password selector, and credits/high-score screen were all
being presented as 320x240. Their assets do contain 240 rows per plane, but—as
with the ending corrected in milestone 102—that is backing storage rather than
the visible height.

All three screens use the original `$EE4A` Copper builder:

| Screen | Original setup | Visible viewport |
| --- | --- | --- |
| Flood title | call at `$9704` | 320x208 |
| PLAY LEVEL/password selector | call at `$97B2` | 320x208 |
| Credits/high-score screen | `$9FF8` setup path | 320x208 |

The builder writes `DIWSTRT=$2C81` and `DIWSTOP=$FCC1`, which exposes 208
scanlines. Its horizontal fetch configuration retains the central 320-pixel
crop from source X=16 through X=335. The horizontal alignment was already
correct and has not changed.

The poster-based copy-protection screen was already configured as 320x208 and
required no correction. The Bullfrog intro remains a separate 320x240 canvas
containing its centered native 160x96 display window.

## Correction

Milestone 103 separates visible height from backing height for all three
affected screens:

- SDL textures, logical stage dimensions, window sizing, and fades now use
  320x208;
- the underlying 352x240 planes remain unchanged for exact source storage and
  front-end compositing;
- the post-credits PLAY LEVEL overlay continues to share the complete
  `TEMPFILE` planes before presenting their common 320x208 crop.

## Regression coverage

- each affected visible viewport is fixed at 320x208;
- each plane remains 44 bytes x 240 backing rows (`$2940` bytes);
- title, four selector states, and four high-score/editor states have new exact
  hashes over the 320x208 visible crop;
- the SDL stage constants for title, selector, high scores, and ending all
  remain 320x208;
- post-score background retention and panel compositing remain covered.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
