# ReFlood Milestone 155: original looping water sound

## Result

Milestone 155 restores the original effect-track loop control used by the
continuous water sound. Touching water starts the same paired effects as
before, but the sustained component now repeats instead of stopping when its
tracker bytecode reaches its first terminator.

## Original-code comparison

The player-water path at `$D1E8-$D220` requests effects 13 and 15 on the first
positive water contact. The original effect routing table gives effect 15 the
control word `$00A4`.

In the original audio dispatcher:

- `$16DF2` copies control bit 5 (`$20`) into the selected channel state;
- `$172A0` checks that flag when the effect track reaches `$FF`;
- `$172E2-$172EC` restores the initial track pointer and continues playback.

The loop is not explicitly stopped when Quiffy leaves the water. It continues
until another effect replaces that Paula channel, the audio state is reset, or
the cavern ends.

## Host correction

`FloodAudioChannel` now stores an effect track's initial pointer and its
original loop flag. `start_track()` derives the flag directly from control bit
`$20`. At an effect-track `$FF`, the player returns to the stored start when
that flag is set. Ordinary finite effects retain their existing termination
behavior, and presentation-music sequence looping remains separate.

This is a general implementation of the original audio control bit; there is
no level-specific or water-sound-specific exception.

## Verification

- Strict C99 core test suite: pass.
- UndefinedBehaviorSanitizer core suite: pass.
- SDL input/settings suite: pass.
- Headless smoke runs for all 42 caverns: pass.
- New audio regression confirms effect 15 wraps to its initial track and
  remains active.
- Existing regression confirms the finite four-channel exit effect still
  reaches an inactive state.
- Release `data/` directory remains present and empty.

The package is source-only and contains no executable, object file, build
directory, or copyrighted Flood game data.
