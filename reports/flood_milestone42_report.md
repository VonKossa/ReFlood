# Flood Milestone 42 — complete Quiffy pose-state selector

## Recovered state inputs

`$D4F2-$D60C` is now represented as a pure, testable state transformation.
It consumes:

- final contact word `$17E68`;
- pre-fallback contact word `$17E6A`;
- contact population `$E1D6`;
- raw joystick Y `$17E7A`;
- animation phase `$180B4`;
- horizontal/vertical banks `$180B6/$180B8`;
- previous pose `$E1E0`;
- the nonzero-material result from `$109A6`.

`$17E6A` is written at `$F41A`, before attribute-bit-7 support fallback may
replace the final ring with S/SE/SW. `FloodPlayer.raw_contact_mask` preserves
this distinction.

## Pose families

The special renderer adds global sprite base `$50` to the selected code:

| Condition | Pose code | Sprite family |
| --- | --- | --- |
| South | `$00-$07` | `$50-$57` |
| North | `$08-$0F` | `$58-$5F` |
| West | `$10-$17` | `$60-$67` |
| East | `$18-$1F` | `$68-$6F` |
| No contact, material present | `$20-$27` | `$70-$77` |
| South with positive raw Y | `$4C-$53` | `$9C-$A3` |
| Recognized single diagonal | `$56-$5D` | `$A6-$AD` |

Each ordinary family combines four animation phases with a bank of 0 or 4.
South overwrites a preceding north selection and returns immediately; west is
then tested before east. Contact auxiliary bit 9 sets the vertical bank to zero
before side-family selection. If neither contact nor material applies, the
previous code is retained while the special renderer remains inactive.

## Integration and tests

`flood_select_quiffy_pose()` exposes the binary state transformation directly.
The SDL-facing sprite choice now distinguishes pose code from pose activation,
so sentinel `$55` can be retained without accidentally selecting sprite `$A5`.

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` passes. Tests
cover the complete single-corner matrix, every cardinal family, north/south and
west/east priority, positive-Y south selection, auxiliary-bit-9 orientation,
material selection, previous-state retention, and special renderer conversion.

## Next step

Quiffy's contact-driven movement and pose selection are now translated through
`$D60C`. The next bounded subsystem is runtime state 22 at `$1363A`: the
horizontal bolt launcher/projectile trap, including its 32x8 sweep, eight-pixel
advance, terrain/player impact state, and reset to its saved emitter origin.
