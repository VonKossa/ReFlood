# Flood reverse engineering -- Milestone 13

## Result

The core player-water mechanics are now substantially reconstructed.

### Player movement globals

The main movement path stores:

- `$180AC` = X
- `$180AE` = Y
- `$180B0` = horizontal motion (`dx`)
- `$180B2` = vertical motion (`dy`)

### Life Force and air

Initialization code writes:

```text
$E1D0 = $01FF = 511
$E1D2 = $003F = 63
```

The player-water code proves `$E1D2` is an air/breath reserve: it falls while sufficiently underwater and recovers out of deep water. Once it becomes negative, `$E1D0` loses 4 units repeatedly. `$E1D0` is therefore confirmed as Life Force and `$E1D2` as air/breath.

### Water produces actual buoyancy

The `$DC4A/$DC82` pair converts water fill height into signed vertical acceleration using two adjacent vertical samples:

```text
dry / dry             -> +2
surface straddle      ->  0
submerged / submerged -> -2
```

So Flood implements gravity and buoyancy with a compact two-point water-surface test.

### Swimming drift

The `$DD58` call site forces vertical speed to `+2` when water is present, the player is in the normal movement mode, and no upward input is being supplied. This is consistent with a gentle sinking/drift behavior while swimming.

### Drowning threshold

At `$DEB8`, a fill height of at least 8 pixels activates the air-consumption path. A game tick/gating global `$17E86` controls individual air decrements. When air drops below zero, Life Force is reduced by 4. When fill height falls below 8, air recovers by 2 toward its normal 63-unit reserve.

### One-shot water event

`$D1F4` detects the first positive water query at the player and invokes `$165DC` with IDs 13 and 15, then sets `$D24E`. This appears to be a one-shot water-entry/audio event; the exact audio meaning remains unlabelled.

## New high-confidence symbols

| Address | Name | Confidence |
|---|---|---|
| `$180AC` | `player_x` | high |
| `$180AE` | `player_y` | high |
| `$180B0` | `player_dx` | high |
| `$180B2` | `player_dy` | high |
| `$E1D0` | `life_force` | very high |
| `$E1D2` | `air_reserve` | very high |
| `$DC4A/$DC82` | water surface force samples | high |
| `$DEB8` | breath/drowning update | high |

## Engine model now

```text
water-state grid
      |
      v
state -> fill height ($109A6)
      |
      +--> two-point surface test -> gravity / neutral / buoyancy
      |
      +--> swim drift rule
      |
      +--> >= 8 px -> consume air -> Life Force damage after air expires
```

## Next target

The most useful next target is the player-state block around `$CE00-$D1xx` and `$D7xx-$DBxx`. It initializes and updates the movement mode globals that surround this water code. Decoding it should turn the remaining anonymous player fields (`$180B4`, `$180B6`, `$180B8`, `$180BA`, `$17E68`, `$E1D4`) into concrete states such as grounded, jumping, climbing, swimming, stunned, or dying.
