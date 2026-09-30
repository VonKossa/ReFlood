# Flood reverse engineering — Milestone 59

## Result

The reconstruction now has Quiffy's weapon/action system: the original
eight-record pool, all 18 dispatcher states, edge-triggered firing, six pickup
entry states, weapon swapping, original action sprites, sounds, timers, object
hits, and SDL compositing. The `$EA` radial-flame family is implemented but is
proven dormant in the released 42 levels.

The game manual names five shipped weapons—flamethrowers, grenades,
boomerangs, shuriken, and delayed-action dynamite—which agrees with the five
pickup families found in level data. Source:
<https://www.lemonamiga.com/doc/flood/666>.

## Record pool and fire edge

`$CB20` enters the action dispatcher at `$CB40`. It visits eight records from
base `$7D0F6`, advancing `$12` (18) bytes after each visit. Each record consists
of nine consecutive big-endian words:

| Offset | Host field |
| ---: | --- |
| `+$00` | X |
| `+$02` | Y |
| `+$04` | DX |
| `+$06` | DY |
| `+$08` | animation / family scratch |
| `+$0A` | dispatcher state |
| `+$0C` | auxiliary word |
| `+$0E` | timer / family scratch |
| `+$10` | target / family scratch |

The C `FloodAction` structure is statically exercised as exactly 18 bytes.

The player path at `$13E42-$13EB8` increments fire counter `$17E5A` while fire
is held, saturating at `$FF`, and clears it on release. State 0 creates an
action only when this counter equals 1, copies selected entry state `$E1EE`,
and increments the counter again. Consequently one press initializes at most
one free record. Any nonzero Quiffy death mode forces the counter to 2 and
blocks creation.

## Complete state map

The jump table at `$CB58` has exactly 18 entries:

| State | Entry | Meaning in the host |
| ---: | ---: | --- |
| 0 | `$CB7C` | idle / allocate on fire edge |
| 1 | `$CB9E -> $D61C` | grenade creation |
| 2 | `$CBB4 -> $D678` | grenade flight and bounce |
| 3 | `$CBBE -> $1273A` | grenade blast |
| 4 | `$CBC8 -> $106A0` | boomerang creation |
| 5 | `$CBDE -> $106E8` | boomerang scan |
| 6 | `$CBE6 -> $104FA` | boomerang launch |
| 7 | `$CBEE -> $10548` | boomerang flight / target |
| 8 | `$CBF6 -> $14F2E` | shuriken creation |
| 9 | `$CBFE -> $15056` | shuriken ricochet flight |
| 10 | `$CC06 -> $1514E` | dynamite placement |
| 11 | `$CC0E -> $151A6` | dynamite fuse |
| 12 | `$CC16 -> $15252` | dynamite blast |
| 13 | `$CC1E -> $15346` | held flame creation |
| 14 | `$CC26 -> $15432` | visible held flame beam |
| 15 | `$CC2E -> $153EA` | alternate held flame path |
| 16 | `$CC36 -> $14EC0` | radial-flame creation |
| 17 | `$CC3E -> $14F8A` | eight-direction radial flame |

A state written by a handler is not redispatched until the record's next game
tick. The host retains that property by taking a state snapshot before its
switch.

## Pickup selection and swapping

The pickup routine at `$FDE8-$1000A` produces these exact selected states:

| Terrain icon | Selected state | Sprite family |
| ---: | ---: | --- |
| `$DD` | 1 | `$0C-$13`, blast `$98-$9B` |
| `$DE` | 4 | `$20-$27` |
| `$EB` | 8 | `$31-$32` |
| `$EC` | 10 | `$35-$36`, blast `$94-$97` |
| `$ED` | 13 | `$37-$3A` |
| `$EA` | 16 | `$18-$1F` |

`$10106-$10166` performs the reverse mapping. On a new pickup, the old weapon
icon is written back to the contacted map cell instead of being discarded.
The host now mirrors that swap and requests pickup sound 16.

## Released-level audit

The complete static maps contain 67 weapon pickups:

- `$DD`: 20, in levels 1, 2, 4, 5, 7, 9, 16-19, 21, 27, 28, 31, 32,
  35, 36, 39-41.
- `$DE`: 14, in levels 3, 4, 11, 14, 19, 27, 29, 33-35, 38, 40-42.
- `$EB`: 8, in levels 9, 10, 12, 21, 22, 33, 34, 41.
- `$EC`: 9, in levels 2, 6, 13, 20, 24, 25, 30, 31, 41.
- `$ED`: 16, in levels 1, 4, 8, 9, 11, 13, 14, 16, 19, 21-23, 26, 32,
  36, 41.
- `$EA`: 0.

Active trigger rectangles contribute 25 `$DD` entries (24 in level 26 and
one in level 32), one `$EC` entry in level 15, and one `$ED` entry in level
27. No active trigger contains `$DE`, `$EB`, or `$EA`. Thus State 16 is real
resident code but unreachable from shipped static and trigger inventory.

## SDL integration

The input loop now maintains the recovered fire counter and invokes action
updates before the ordinary object dispatcher. A bounded 64-entry render queue
allows one logical action record to emit several sprite pieces, which is
required by the grenade explosions, flame beam, and radial flame. The SDL
renderer composites this queue before the object layer. Held flame also selects
the recovered weapon-pose phase from the Quiffy sprite bank.

Projectile impacts advance the struck object's state, clear its animation byte,
and award the family score. Grenade, shuriken, and dynamite lifecycles have
dedicated regressions; radial flame has a singleton regression; pickup tests
cover all six mappings, old-weapon swap, and one-record-per-press allocation.

## Verification

- GCC 13.3, C99, `-Wall -Wextra -Wpedantic -Werror`: pass.
- Core regression executable: `core ok`.
- UBSan core regression: pass with no diagnostic.
- Normal and UBSan headless hosts: all 42 levels complete their 300-tick run.
- Preview raster: 1320 x 820 RGB PNG, decoded with the verified gameplay
  palette and natural ascending bitplane significance.

## Remaining accuracy boundary

The dispatcher topology, layout, creation edge, pickup mappings and swap,
state transitions, timers, sound IDs, and sprite families are recovered from
the original instructions. The compact host's grenade blast spread and
boomerang target scan/homing are faithful behavioral integrations, but they
are not yet instruction-for-instruction translations of every helper and
collision rectangle. That narrow microgeometry boundary is the appropriate
Milestone-60 target; it does not leave any dispatcher state missing.
