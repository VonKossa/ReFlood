# ReFlood — Milestone 140

## Pause and restart controls

Milestone 140 implements the two missing active-gameplay keyboard commands:

- `P` pauses the game. The current cavern frame remains displayed and no game
  update is performed while paused. The original waits first for fire to be
  down and then for fire to be released; gameplay resumes on that release.
- `R` fades the current cavern to black, reloads it through the ordinary level
  setup, displays the complete 51-frame level banner, and only then deducts one
  life. The score and global frame count survive; terrain, player state,
  objects, actions, water, mechanisms, weapons, and effects return to their
  level-entry state. If the deduction reaches zero, Life Force is forced fatal
  and the ordinary falling/cross death sequence handles game over.

Both commands are SDL key-down edges with keyboard repeat ignored. Holding `R`
therefore cannot consume another life after the level banner, and holding `P`
cannot repeatedly enter pause. Escape retains its separate final-life death
sequence.

## Original-code evidence

The active player path at `$D308` decodes the raw CIA keyboard byte stored at
`$17F2A`:

- `$D9` decodes to Amiga raw key `$13` (`R`). At `$D33C-$D362` the game calls
  reset `$DF54`, calls cavern setup `$A3D2` (whose entry performs the recovered
  fade and whose path includes the level banner), subtracts one from lives,
  and writes `-1` to Life Force only when the result is not positive.
- `$CD` decodes to Amiga raw key `$19` (`P`). The loops at `$D370-$D38A` poll
  until the primary fire counter becomes nonzero and then poll until it returns
  to zero. They perform no gameplay update between those polls.

## Verification

- `P` and `R` map only in active keyboard input mode and ignore repeat events;
- a paused simulation waits for the original fire-down/fire-up handshake
  without advancing game state;
- restart fades, restores the same level from disk, shows its banner, and then
  deducts exactly one life;
- score and global tick count survive restart;
- player gauges, location, terrain, runtime records, weapons, and mechanisms
  return to their level-entry state;
- restarting on the final life clamps lives to zero and enters the normal death
  sequence;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
