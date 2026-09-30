# Flood reverse engineering -- Milestone 12

## Result

The water state's visual/physical meaning is now substantially decoded.

### `$109A6`: water fill-height lookup

`$109A6` samples the water grid at the tile containing `(x+8, y+8)`:

```c
index = (((y + 8) >> 4) * 128) + ((x + 8) >> 4);
state = water[index];
```

If the state is zero, it returns zero. Otherwise it indexes a 40-byte table at `$109E8`.
If the table entry is non-zero the function returns:

```c
0x00010000 | fill_height
```

The fill heights are `1, 2, 4, 6, 8, 10, 12, 14, 16` pixels.

### Water fills a tile from the bottom upward

Callers around `$DC4A/$DC82` use the low 4 bits of Y (`y & 15`), add the returned fill height, and compare against 16. This is equivalent to asking whether the sampled pixel lies in the bottom `fill_height` pixels of a 16-pixel tile.

This gives the state ladder a concrete meaning: it is not only propagation scheduling; many states encode progressively rising visual/physical water height within a tile.

### State groups

The lookup table groups most active states into repeated fill levels:

- states 10-12 -> 2 pixels;
- 14-16 -> 4 pixels;
- 18-20 -> 6 pixels;
- 22-24 -> 8 pixels;
- 26-28 -> 10 pixels;
- 30-32 -> 12 pixels;
- 34-36 -> 14 pixels;
- 38 -> 16 pixels.

Interleaved states map to zero and are therefore propagation/transition phases rather than visible fill levels through this helper.

### Correction to Milestone 11

The `$131AE-$13296` region is not primarily a renderer. It is object/water interaction logic. The previous next-target hypothesis was wrong and has been corrected rather than forced.

Also, state 40 should no longer be called simply "settled water". `$F630` does treat it as unscheduled/terminal, but `$109A6` only has entries 0..39. A safer current label is `scheduler_terminal_state_40` until its conversion/cleanup path is traced.

### Engine model now

```text
water propagation states ($F698)
        |
        +--> water-grid state 0..39
                 |
                 +--> $109A6 state->fill-height table
                            |
                            +--> 0/1/2/4/.../16px fill
                            |
                            +--> player/object water tests
```

## New symbols

| Address | Proposed name | Confidence |
|---|---|---|
| `$109A6` | `WaterFillAtPoint` | high |
| `$109E8` | `water_state_fill_height[40]` | high |
| `$1316E` | `WaterCellOccupiedAtObjectCoord` (provisional wording) | medium-high |
| state 40 | `scheduler_terminal_state_40` | high as scheduler role; visual role unresolved |

## Next target

Trace the callers of `$109A6` around `$D1F4`, `$D5E4`, `$DC4A/$DC82`, `$DD58`, and `$DEB8`. These are likely where water affects Quiffy's movement, damage, buoyancy/swimming, and/or death state. That should let us reconstruct the player-water interaction rather than only the flood simulation.
