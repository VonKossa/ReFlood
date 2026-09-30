# Flood reverse engineering — Milestone 7

## Result

This milestone completes the gameplay handler at **$13972** and identifies the helper at **$10DCA**.

### $10DCA: player AABB overlap

`$10DCA` is a four-argument rectangle-vs-player overlap test:

```c
int16_t overlaps_player(int16_t x, int16_t y, int16_t width, int16_t height);
```

It derives Quiffy's collision box from globals `$180AC/$180AE`, offsets it by `(8,8)`, and uses a fixed **16x16** player collision rectangle. On overlap it returns `1` and sets `$10DC8 = 1`; otherwise it returns `0`.

The `$13972` caller tests a **32x32** object box and subtracts **8** from `$E1D0` on contact.

### $E1D0: life force

`$E1D0` is now high-confidence Quiffy **life force**. Evidence includes:

- initialized/reset to `$01FF` (511),
- multiple enemies/hazards subtract differing amounts (`8`, `10`, `16`, `40`, etc.),
- some code adds `$40` and clamps back to `$01FF`, matching a health-restoration mechanic,
- the game manual explicitly describes a Life Force bar depleted by dangerous creatures and restored by hearts.

### $13972: ricocheting damaging enemy

The handler has a coherent complete behavior:

1. Load `x,y,dx,dy` from object offsets `+00,+02,+04,+06`.
2. During an initial phase (`+08 == 0`), accelerate upward by `-1/frame`.
3. Clamp upward speed to `-12`, then switch `+08` to normal gravity mode.
4. In normal mode add `+1/frame` to vertical speed, capped at `+16`.
5. Query the tile map using `$F0E0(x,y,dx,dy,32,32)`.
6. Reverse `dx` when collision flag bit 1 is present on the X edge.
7. Bounce `dy` when collision flag bit 1 is present on the Y edge.
8. Integrate position and store the new motion terms.
9. Draw fixed sprite `$AE` through `$E4AA`.
10. Test a 32x32 overlap against Quiffy via `$10DCA`.
11. On contact, reduce life force by 8.

## Likely original game entity: Beady Ball

This handler is a very strong behavioral match for the **Beady Ball** described in the Flood manual: a creature born at high velocity that "ricochet[s] uncontrollably" from things it contacts. The code's wall reversal, vertical bouncing, and contact damage fit that description directly.

This identification is still marked **inferred** because the compiled binary contains no surviving source-level symbol saying "Beady Ball".

## Recovered type-specific record

For this handler family we can now use:

```c
typedef struct FloodBeadyBall {
    int16_t x;      // +00
    int16_t y;      // +02
    int16_t dx;     // +04
    int16_t dy;     // +06
    int16_t phase;  // +08
} FloodBeadyBall;
```

This reinforces the Milestone 5 conclusion that Flood has a common object prefix but handler-specific field semantics.

## Vertical bounce detail

The original floor-hit logic is slightly unusual and is preserved in the C lift:

- downward `dy` is halved;
- if the halved value is below `2`, the bounce is forced to `-12`;
- otherwise the halved value is negated;
- upward impacts simply negate `dy`.

This is not generic modern physics; it is a faithful semantic reconstruction of the original handler.

## New source files

- `src/beady_ball_13972.c` — full gameplay lift of `$13972`
- `include/flood_beady_ball.h` — typed object/runtime interface
- `src/player_collision_10dca.c` — exact semantic lift of `$10DCA`
- `include/flood_player_collision.h` — player collision state/API
- `semantic_disassembly_13972.s` — annotated original control flow
- `semantic_disassembly_10dca.s` — annotated player AABB helper

Both reconstructed C source files compile with strict warnings enabled.

## Recommended next milestone

The best next target is the cluster immediately following `$13972`, particularly `$13A4C` and `$13BA8`.

Why:

- `$13A4C` shares the same object prefix and collision system but has animation/state logic tied to `$17E86`.
- It also calls `$10DCA`, proving it interacts with Quiffy.
- `$13BA8` is conditionally invoked from it and is therefore likely a secondary state transition or interaction routine.

Recovering that cluster should reveal another named enemy/object and further clarify the common gameplay record.
