# ReFlood — Milestone 129

## Actual “Let's go” trigger corrected

Milestone 128 moved the wrong trigger. It delayed the host's explicit sound-7
injection, but a real level load sets `player.invulnerable_timer` to 100. On
the first banner update, `update_player()` therefore queues sound 52 (`$34`).
That paired dispatch is the audible level-start cue and continued to play as
the banner appeared.

Milestone 129 makes two corrections:

1. Sound `$34` queued by the banner's first gameplay update is withheld until
   the complete 51-frame banner and its final dwell have finished.
2. The separate sound-7 injection is removed. The original value 7 at `$AD7C`
   selects resource `W_BLOCKS.BLK`; it is not an audio request.

Only the level-entry `$34` request is deferred. Any unrelated effects queued
during the banner keep their existing dispatch timing, and ordinary sound
`$34` requests outside the banner are unchanged.

## Code trace

- `reset_level_runtime()` mirrors the original level-entry setup and assigns
  protection timer 100.
- `$AD94-$ADB2` performs the first gameplay update at the beginning of the
  banner sequence.
- `$D07C-$D0A2` tests the protection countdown and calls the original effects
  driver `$165DC` with `$34` when the value is 100.
- ReFlood now captures that initial `$34` request and dispatches it at the
  banner-to-gameplay boundary.

## Verification

- strict C99 core build with warnings treated as errors: pass;
- complete core regression suite: pass;
- SDL host regression suite: pass;
- the banner regression now loads Level 1, verifies protection timer 100, and
  compares both resulting audio tracks against a direct sound-52 dispatch;
- both cue channels remain inactive throughout the banner and become active
  only after the 51st normal-speed dwell, at 3,060 ms: pass.

The package remains source-only; no executable is included.
