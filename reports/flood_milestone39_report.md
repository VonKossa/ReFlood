# Flood Milestone 39 — Quiffy attachment state

## Recovered player state

The original control path `$D7CE-$DF52` distinguishes values which the earlier
host conflated:

- `$17E78/$17E7A`: freshly polled joystick X/Y, values -1, 0 or +1;
- `$E1DA/$E1DC`: captured surface X/Y directions;
- `$E1D6`: population count of the eight contact bits;
- `$E1D8`: attachment mode (1 floor/ceiling, 2 wall);
- `$E1DE`: attachment/corner timer, cancelled after ten updates;
- `$180B0/$180B2`: actual X/Y motion;
- `$180B6/$180B8`: horizontal/vertical orientation banks.

`FloodPlayer` now represents the raw and captured directions separately.

## State-machine integration

`flood_maintain_quiffy_attachment()` is a portable reconstruction of
`$DAEC-$DBE6`. It:

- counts and classifies the current contact ring;
- cancels the tangent component when only one support remains;
- releases an attachment when two or more contacts make the captured tangent
  zero;
- applies the original wall-mode Y alignment (`y += y & 3`);
- maintains the ten-update timeout and clears it outside attachment mode.

The cardinal resolver from `$D99E-$DAEA` now reads raw joystick directions and
writes separate captured surface directions. Attached movement uses the
captured pair at the original `*4` scale; unattached movement uses raw input.
First contact retains the original `(y + 2) & ~3` vertical snap.

The free-flight vertical update now follows `$DC4A-$DCC4`: two water-edge
tests contribute -1 or +1, then velocity damps one step toward zero. In air
this produces the binary's net +1 gravity step rather than the old host's +2.
The `$DF20` supported vertical-command multiplier is integrated, as are the
attribute-bit-2 slope exceptions and iterative Y collision reduction from
`$DDAA-$DE30`.

Auxiliary contact bit 8 now feeds the death path unless invulnerability is
active. The invulnerability timer has an explicit player field and decrements
once per game tick.

## Tests and visual aid

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` succeeds. Tests
cover all four cardinal attachment modes, south/east contact triplets, contact
population, attachment cancellation, wall alignment, the ten-update timeout,
and the established collision and asset checks.

`quiffy_contact_states.png` documents all ring bit numbers and cardinal/triplet
patterns used by the reconstructed code.

## Remaining accuracy boundary

The contact ring, cardinal selection, attachment maintenance, free-flight
gravity/water-edge calculation, and primary slope response are direct
translations. The `$F4B2/$F4DE` attribute-bit-4/5 micro-corrections,
jump/parachute/balloon edge cases and orientation-specific sprite selection
still require complete integration.
