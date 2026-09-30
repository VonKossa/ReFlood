# Flood Milestone 40 — Quiffy edge mechanics and frames

## Exact additions

- `$F4B2/$F4DE` are now integrated. On solid diagonal Y support, attribute
  bits 4 and 5 apply opposite one-unit X corrections at the NW and SE probes.
- `$E1BC` is identified as the mine knockback counter, not a generic jump
  flag. A mine writes 3; `$D8D8` decrements it, forces joystick Y to -1, and
  retains only south support plus auxiliary death bit 8.
- Any contact clears the mutually exclusive parachute (`$E1B6`) and balloon
  (`$E1BA`) timers. Death clears both timers and the mine impulse.
- Balloon motion forces Y velocity to -4. Parachute fall caps remain 2, 4, or
  8 according to vertical input; ordinary velocity remains clamped to 16.
- Horizontal and vertical orientation banks track the signs of final X/Y
  motion. Zero motion preserves the previous bank.

## Verified player frames

The ordinary Quiffy frame is exactly:

`$C8 + (animation_phase & 3) + horizontal_bank`

where the horizontal bank is 0 or 4. Thus `$C8-$CB` and `$CC-$CF` are the two
four-phase facing banks. The same phase-plus-horizontal-bank value is written
to Aunt Matilda's delayed history. The vertical bank belongs to the broader
contact/effect animation selector and is not folded into the base Quiffy ID.

`quiffy_frames_milestone40.png` renders all eight records using the confirmed
gameplay palette, natural plane significance 0/1/2/3, and the fifth plane as
mask.

## Validation

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` passes. Tests
now cover both slope attributes at both special diagonal probes, contact
cancellation of carried items, the three-update mine impulse, vertical facing,
and all eight verified frame IDs in addition to Milestone 39's contact suite.

## Remaining boundary

The exact contact and normal-motion core is now substantially reconstructed.
The next binary work should trace the broader `$D460-$D4F0` contact/effect
animation selector (`$E1E0`) and connect actual Mine, parachute, and balloon
object pickups to these now-proven player-state paths.
