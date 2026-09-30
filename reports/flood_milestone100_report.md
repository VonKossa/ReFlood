# Flood reconstruction — Milestone 100

## Original high-score text colors

The high-score board's text had the correct shapes but the wrong color
distribution: its upper shading was missing the stronger red visible in the
original.

Disassembly separates two similar glyph paths:

| Original path | Plane source order | Purpose |
| --- | --- | --- |
| `$12B4E-$12BA8` | `+2,+4,+6,+8` | Fixed names and score digits |
| Live editor glyph | `+2,+4,+8,+6` | Animated character at the name-entry cursor |

The reconstruction had used the live editor's swapped third/fourth source
words for every fixed board glyph. That exchanged the relevant red/orange and
purple color bits. Milestone 100 restores the natural plane order for fixed
text while leaving the animated editor character unchanged.

## Regression coverage

- a known upper-glyph pixel resolves to palette index 11 (`$600`, dark red);
- the non-qualifying-score board has an exact full-screen pixel hash;
- the qualifying-score board has an exact full-screen pixel hash;
- the active character's vertical animation retains its distinct exact hash;
- the completed `COD` entry redraws with the fixed-board plane order and has an
  exact full-screen pixel hash.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
