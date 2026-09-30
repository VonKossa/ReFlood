# Flood Milestone 47 — complete runtime State 12

The object dispatcher at `$CC60` selects `$10A92` for runtime state 12. That
routine branches on record word `+14` (`+0E`) into four named subtypes created
by the marker initializer.

| Subtype | Enemy | Handler | Sprite frames | Contact effect |
| ---: | --- | ---: | --- | --- |
| `$30` | Lumpy Wanderer | `$10AC8` | `$80-$87` | life −16 |
| `$38` | Psycho Teddy | `$13A4C` | `$88-$8F` | immediately fatal |
| `$66` | Snail | `$10B86` | `$B6-$BD` | life −16 |
| `$6E` | Vong | `$13CA0` | `$BE-$C5` | immediately fatal |

## Shared walkers

Lumpy and Snail have instruction-equivalent handlers. Gravity adds one each
update. `$10D5E` performs a swept 32×32 collision query, cancels blocked
vertical movement, constructs direction-sensitive side/corner bits, and uses
those bits to reverse horizontal velocity or continue around a ledge. Animation
advances on alternating display frames through `$17E86`. The sprite is drawn at
`(x,y+2)` before the new position is committed.

## Psycho Teddy

Teddy adds two gravity units, capped at 16, and probes a centered 16×16 core at
offset `(8,16)`. Its byte-8 mini-state produces the wall-jump/reversal sequence.
It tests Quiffy with a 24×24 box at `(x+4,y+8)` and sets life to −1 on contact.
When food remains, `$13BA8` consumes tile `$94` or `$24-$27` at `(x+16,y+16)`.
A support-attribute probe at `(x+16,y+28)` gates movement. Frames are selected
from `$88-$8F` using the retained horizontal direction.

## Vong

Vong also adds two gravity units capped at 16. A one-pixel downward 24×32 probe
detects support. While the map contains 1–48 food tiles, a signed RNG remainder
test gives a chance to create one of `$94,$24,$25,$26,$27` in an empty cell at
`(x+16,y+16)`. A second swept probe handles actual motion. Horizontal impact
flips the stored velocity for the next update while the current local velocity
still advances once, matching the original ordering. Its 24×32 player contact
box is lethal.

## Host integration

- Loader reproduces `$E6BC` and counts the five food tile IDs before marker
  initialization.
- SDL renders all State-12 objects and preserves their exact pre/post-movement
  draw timing through separate render coordinates.
- Tests cover both shared walkers, direction reversal, alternating animation,
  Teddy food consumption and jump state, Vong deterministic food creation and
  delayed reversal, both damage classes, and the level food counter.
