# Flood reverse engineering — Milestone 15

## Scope

This milestone analyzes the player animation-selection bridge around `$D460-$D60E`.

## High-confidence findings

### `$D460`: diagonal corner-transition selector

`$D460` reads the low eight bits of `$17E68`, already identified as the clockwise 8-neighbor surface-contact ring. It only examines the diagonal bits:

- bit 1 = NE
- bit 3 = SE
- bit 5 = SW
- bit 7 = NW

It starts from frame selector `$55` and, based on `$180B6/$180B8`, returns one of the selectors `$55-$5D`. `$55` acts as the no-override/default value; the other values are a dedicated bank of corner-transition poses.

This is strong evidence that Flood contains explicit corner-climbing art/animation rather than simply rotating a generic walking frame.

### `$D4F2`: main movement-frame selector

`$D4F2` first checks `$E1D6`, the popcount of the eight surrounding contact bits. When exactly one contact exists, it tries `$D460`; a valid diagonal-corner result bypasses the normal selector.

Otherwise, `$D4F2` chooses between several animation banks using `$17E6A` and the player animation fields:

| `$17E6A` bit | selector base | primary bank field | interpretation |
|---|---:|---|---|
| 0 | `$08` | `$180B6` | one horizontal-surface orientation |
| 4 | `$00` / `$4C` | `$180B6` | opposite horizontal-surface orientation / special downward-input bank |
| 6 | `$10` | `$180B8` | one vertical-wall orientation |
| 2 | `$18` | `$180B8` | opposite vertical-wall orientation |

The final selector adds `$180B4`, the animation phase, and in several paths twice the orientation-bank value.

When none of those cardinal classes is active, the routine calls `$109A6` at the player's `(x,y)`. If water is present it switches to a base `$20` frame bank. This is the first direct link between the recovered water-depth helper and Quiffy's animation selector.

### Frame cache

The selected value is written to both `$E1E0` and `$E1E2`. Earlier caller analysis around `$CE80` shows `$E1E2` is consumed as a cached/forced player animation selector. These are therefore best named as player frame-selector cache fields, not general gameplay state.

## Updated player-animation field map

- `$180B4` animation phase
- `$180B6` horizontal-surface/orientation animation bank
- `$180B8` vertical-surface/orientation animation bank
- `$E1E0` current frame-selector cache
- `$E1E2` forced/cached frame selector used by the renderer
- `$17E68` surrounding surface-contact mask
- `$17E6A` cardinal movement/contact classification mask
- `$E1D6` number of active surrounding contacts

## What remains provisional

We have recovered frame-selector numbers and movement semantics, but not yet the actual artwork represented by every selector. Calling `$55-$5D` 'corner-transition frames' is high confidence; assigning exact visual labels such as 'floor-to-right-wall frame 2' requires decoding the player sprite table/art data.

Likewise the `$00/$08/$10/$18/$20/$4C` banks are clearly distinct movement contexts, but exact left/right/up/down pose names should be verified against sprite graphics rather than inferred solely from control flow.

## Next target

Decode the player sprite-index/art lookup used by the renderer around `$CE80-$D070`, then connect these frame selector values to concrete sprite descriptors. That should let us name the movement banks visually and produce the first full Quiffy animation enum.
