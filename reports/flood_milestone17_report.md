# Flood reverse engineering - Milestone 17

## Result

The two timed effects represented by sprites `$C6/$C7` and globals `$E1B6/$E1BA` are now identified from their pickup IDs and their direct effect on Quiffy's vertical physics.

### `$E1B6` = Parachute timer

Pickup object id `$E7` loads `$E1B6` with `1000`, clears the Balloon timer, and plays sound `$26`.

While active, the Parachute limits Quiffy's *downward* vertical speed. The normal cap is `+4`. Joystick vertical input changes it:

- up (`-1`) -> cap `+2` (slowest descent)
- neutral (`0`) -> cap `+4`
- down (`+1`) -> cap `+8` (faster descent)

This exactly matches the manual description: the Parachute gives a slow ride down and pushing up slows the descent further.

### `$E1BA` = Balloon timer

Pickup object id `$E8` loads `$E1BA` with `1000`, clears the Parachute timer, and plays sound `$25`.

While active, the player-physics path sets the vertical component directly to `-4`, producing a steady upward ride. This matches the manual's Balloon behavior.

### Mutual exclusion

The two effects cancel each other on activation. They are not simultaneously active in the normal pickup path.

## Recovered symbols

| Address | Meaning |
|---|---|
| `$E1B6` | Parachute countdown |
| `$E1B8` | working Parachute downward-speed cap |
| `$E1BA` | Balloon countdown |
| `$E7` | Parachute pickup/object id |
| `$E8` | Balloon pickup/object id |
| `$C6` | active Parachute status/render sprite |
| `$C7` | active Balloon status/render sprite |

The `$C6/$C7` identification follows the already recovered renderer: when the corresponding timer is nonzero, that sprite is drawn above Quiffy and the timer is decremented.

## Clean C model

`src/airborne_powerups.c` contains a portable lift of the recovered behavior. It preserves the original numeric velocities and timer values and compiles cleanly with strict warnings.

## Why the identification is high confidence

Three independent pieces agree:

1. Pickup IDs `$E7/$E8` initialize mutually exclusive 1000-tick effects.
2. The physics consumers implement slow descent versus forced ascent.
3. Flood's original manual explicitly describes Parachute as a slow descent (slower when pushing up) and Balloons as a slow upward ride.

## Next target

The neighboring item dispatch table contains Stout, Cocktail, Orange Can, Hearts, Mines, Switch, Droplet, Plunger, Parachute and Balloon behavior. Now that several are anchored by semantics, the next high-value step is to recover that entire object-id -> gameplay-effect table. Doing that should name most remaining item globals and complete a large part of Flood's collectible/power-up subsystem.
