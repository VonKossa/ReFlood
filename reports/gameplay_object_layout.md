# Flood milestone 5 - gameplay object record

## High-confidence common fields

Repeated handlers from `$1363A` through `$13DC8` use `A0` as the current object record.
The following offsets are directly evidenced by many independent routines:

| Offset | Size | Working name | Evidence |
|---|---:|---|---|
| `+00` | word | `x` | loaded before every sprite call; modified by motion/collision paths |
| `+02` | word | `y` | loaded before every sprite call; modified by motion/collision paths |
| `+04` | word | `slot04` | type-specific; copied back to X in `$13714`, but arithmetic state in other handlers |
| `+06` | word | `slot06` | type-specific; copied back to Y in `$13714`, incremented/clamped elsewhere |
| `+08` | byte/word | `flags08` | tested/set/cleared as state/flags; often set to 1 after collision/state changes |
| `+09` | byte | `anim09` | repeatedly used as animation/phase counter and mixed with global `$17E86` |
| `+0A` | word | `timer0a` | reset/set/incremented by several handlers |
| `+0E` | word | `sprite0e` | repeatedly used to derive sprite IDs (`+3`, `+0x50`, etc.) |
| `+14/+16` | words | saved coordinates in at least one object family | written together when a state is initialized |
| `+1D` | byte | state/timer in one family | explicitly initialized from a constant |
| `+20` | word | timer in one family | tested and initialized to 10 |

## Key architectural conclusion

Offsets `+04/+06` **cannot safely be named `vx/vy` globally**. Routine `$13714` restores `x/y` from them when an animation finishes, whereas `$13780`, `$13880`, `$13972`, `$13A50`, and `$13CA0` perform arithmetic on the same slots. Flood therefore appears to use a fixed-size common object record whose tail/payload is interpreted differently by different handler types.

A portable reconstruction should model this as a common header plus handler-specific payload/union once more types are recovered.

## Exact routine `$13714`

This routine is particularly clean and gives us the first nearly exact gameplay handler:

1. Read `x`, `y`, and animation byte `+09`.
2. If phase is zero, play sound ID `7` via `$165DC`.
3. Increment phase and store it back to `+09`.
4. If phase reaches `7`:
   - clear `+08`;
   - clear word `+0A`;
   - clear `+09`;
   - restore `x` from `+04` and `y` from `+06`;
   - return.
5. Otherwise draw sprite `0x98 + (phase >> 1)` at the current position.

That produces the sprite sequence `0x98,0x99,0x99,0x9A,0x9A,0x9B` for phases 1..6 before the record resets on phase 7.

## Other handler families

`$13780` reads `+00/+02/+04/+06`, increments `+06`, advances `+09` using global `$17E86 & 3`, calls `$10D5E`, conditionally changes `+08`, then writes all four words back. This looks like a moving/colliding object family.

`$13880` and `$13972` both clamp a signed value derived from `+06`, call the common collision/geometry helper `$F0E0`, write updated `x/y/+04/+06`, and draw a sprite. They are strong candidates for two related movement modes.

`$13A50` through `$13BA4` is another larger movement/state handler. It updates animation phase from `$17E86`, performs collision checks with `$F0E0`, uses `$F50A`, writes positions back, and derives its sprite from `+0E` plus an offset.

`$13CA0` through `$13DC8` is yet another handler using the same record prefix and the same collision helper. Its sprite is derived from `+0E` with base `0x50`.

## Globals newly reinforced

- `$17E86`: frame/animation tick source. Multiple handlers mask it with `& 1` or `& 3` before advancing animation state.
- `$17E60/$17E62/$17E64`: global directional/mode flags consulted by movement handlers.
- `$E1D0`: global event/collision accumulator; several handlers subtract constants or write `-1` after collision responses.
- `$F0E0`: central geometry/collision helper used by multiple unrelated object handlers.
- `$10DCA`: rectangle/overlap helper; callers pass positions and dimensions such as 8x32, 32x32, 24x24, 32x24.

## Next target

The highest-leverage next routine is `$F0E0`. It is called by almost every movement handler with `(x, y, dx, dy, width, height, ...)`-shaped arguments. Recovering it should turn `slot04/slot06` from vague type-specific slots into actual motion/collision semantics for several object families at once.
