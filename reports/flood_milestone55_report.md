# Flood reverse engineering — Milestone 55

## Result

Milestone 55 replaces the State-9-specific host routine with the complete
shared handler dispatched by runtime States 6, 9, and 15 at `$10A10-$10A90`.
All three state numbers now have identical animation, sound, damage, cleanup,
and SDL rendering behavior.

## Exact sequence

The handler reads unsigned animation byte `+9`. If it is zero, sound 7 is
requested. It then increments the value and writes the low byte back.

| Update | Stored byte `+9` | Frame calculation | Visible sprite | First-frame effects |
| ---: | ---: | ---: | ---: | --- |
| 1 | 1 | `1 >> 1 = 0` | `$98` | sound 7; 32x32 overlap removes 40 life |
| 2 | 2 | `2 >> 1 = 1` | `$99` | none |
| 3 | 3 | `3 >> 1 = 1` | `$99` | none |
| 4 | 4 | `4 >> 1 = 2` | `$9A` | none |
| 5 | 5 | `5 >> 1 = 2` | `$9A` | none |
| 6 | 6 | `6 >> 1 = 3` | `$9B` | none |
| 7 | 7 | cleanup branch | no draw | State=0, word `+10`=0, byte `+9`=0 |

The first-frame damage test occurs after drawing and uses the object's `(x,y)`
as a 32x32 rectangle against Quiffy's centered 16x16 collision body.

## Reachability

### State 9

State 9 is live. Doctor Dusty's adjacent State-10 companion reaches the end of
its countdown at `$1385A-$1387C`, moves upward by 12 pixels, writes State 9 at
`$13870`, and clears byte `+9`. The shared burst begins on the next dispatcher
visit, not during the State-10 update that performs the transition.

The host preserves that one-visit timing by running the existing shared bursts
before the State-1/10 chain in each grouped update tick.

### States 6 and 15

Both are dormant:

- zero static marker-6/15 cells across all 42 maps;
- zero marker-6/15 bytes in every rectangle referenced by active triggers;
- marker 6 and marker 15 initializers only clear the map cell;
- no writes of State 6 or State 15 occur in the recovered resident image.

Their dispatcher entries remain meaningful executable paths and are now
available for exact synthetic testing.

## Implementation

- Renamed the private State-9 helper to the shared burst implementation.
- Added public `flood_update_states_6_9_15()`.
- Removed State 9 from the State-1/10 loop to prevent double updates.
- Ordered the grouped calls to preserve State 10 -> State 9 next-tick timing.
- Added States 6 and 15 to the same SDL cached-sprite rendering path.

## Verification

Focused tests cover all three dispatch values, all six visible frames, first-
frame-only sound and damage, cleanup fields, State-10 transition timing, and
zero shipped reachability for States 6 and 15.

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror`, the complete
core suite, UndefinedBehaviorSanitizer with abort-on-first-error, and all 42
levels for 300 headless ticks each pass.

## Next boundary

Every nonempty handler call in the `$CC60` runtime object jump table is now
represented. The next bounded pass should reconcile the State-24-to-State-12
bridge and prove the no-op entries (States 0, 3, 4, 7, and 23), then return to
the unresolved Sparkling Fungi/static-hazard path.
