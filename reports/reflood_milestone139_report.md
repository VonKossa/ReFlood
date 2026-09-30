# ReFlood — Milestone 139

## Exact original flamethrower lock ordering

Milestone 139 replaces the inferred flamethrower movement rules from milestones
136–138 with a direct translation of the original control-flow ordering.

## Original-code evidence

The relevant original instructions are unambiguous:

- `$D7CE` calls the joystick/fire poll at `$13E06`;
- `$D7E4-$D7F4` tests global word `$E1AC` and, when nonzero, writes zero to
  both joystick axes `$17E78/$17E7A`;
- idle action State 0 at `$CB7C-$CB9A` allocates the selected State 13 on the
  fire edge, increments the fire counter, and does not redispatch that new
  state during the same update;
- State 13 at `$15346` tests south support in the current contact word and sets
  `$E1AC` at `$15366` after successful initialization;
- States 14/15 clear `$E1AC` when the flame action terminates; landing does not
  clear it;
- the free-flight path at `$DC4A-$DCC4` evolves the existing vertical velocity
  even though the joystick axes are zero.

This proves that the lock is tied to the active flame flag, not directly to the
fire counter or geometry.

## Precise jump-and-fire window

The working sequence occupies two gameplay updates:

1. Fire is pressed while supported. State 0 allocates State 13, but `$E1AC`
   remains clear.
2. On the next update, Up is accepted before State 13 runs. Quiffy receives the
   jump velocity, and State 13 sees that update's pre-movement south contact and
   sets `$E1AC`.
3. Beginning with the following update, both joystick axes are suppressed.
   Existing vertical velocity continues upward and gravity later reverses it.

If Up and fire are first pressed on the same allocation update, State 13 is not
visited until the next update and finds that support has already gone. That is
why the original requires precise timing rather than a simultaneous-input
shortcut.

## ReFlood correction

- movement suppression now tests `weapon_pose_active`, the host equivalent of
  `$E1AC`;
- the fire-counter/contact-derived filters from milestones 136–138 are removed;
- the host-only jump and post-landing mobility latch is removed;
- the remembered-support shortcut in State 13 is removed;
- release clears the active flag in action dispatch, after that update's
  movement phase, matching the original ordering.

## Verification

- standing State 13 initializes before the active lock begins;
- all four directions are suppressed once the flame flag is active;
- fire-then-Up performs the valid jump-and-fire sequence;
- Up+fire on the allocation edge fails State 13 on the next unsupported update;
- held directional input cannot steer an active flame jump;
- open-air vertical velocity continues through the apex and descent;
- a ceiling reduces vertical motion to zero and Quiffy remains fixed there;
- landing remains locked while the flame action is active;
- release clears the action and movement resumes on the following update;
- falling-beam continuity and non-flamethrower movement remain passing;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
