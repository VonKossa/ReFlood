# ReFlood — Milestone 123

## Original corner state machine restored

Milestones 117–121 changed the surface-attachment cache in response to the
reported delayed and pixel-perfect turns. Those changes did not make the
intended corner assistance visible because they altered the wrong part of the
original routine.

The original game was run under emulation and its live 68000 code at
`$D99E–$DBE6` was disassembled. The trace established that the attachment
directions are not refreshed from current input. Instead, the captured X/Y
pair is retained for up to ten gameplay updates and copied back into the live
direction words before the next contact is resolved.

ReFlood previously used the cache only when calculating velocity. The contact
resolver therefore saw the new keyboard direction rather than the carried
direction, so the original multi-update corner handoff could not occur.

## Correction

- `$DB14–$DBE6` is now reproduced branch-for-branch for both horizontal and
  vertical attachment modes.
- Cached directions are replayed before `$D99E–$DAEA` resolves the current
  contact ring.
- The original wall Y-alignment, contact-count release conditions, ten-update
  timeout, and timeout reset are retained.
- The speculative direction refresh, immediate perpendicular release,
  cardinal/diagonal exceptions, and wall-top special case from milestones
  117–121 have been removed.
- The independent milestone-118 flamethrower behavior is unchanged.

## Verification

- strict C99 core build with all warnings treated as errors: pass;
- complete gameplay and presentation regression suite: pass;
- SDL input/settings regression suite using the SDL stub: pass;
- exact mode-1 and mode-2 cache replay and release branches: pass;
- integrated block-corner trace reaches the top-of-block jump two gameplay
  updates earlier than milestone 121: pass;
- existing wall-top, jump-and-fire, and falling-flamethrower regressions: pass.
