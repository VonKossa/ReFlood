# Flood Milestone 88: automatic airborne death fall

## Result

Milestone 88 fixes Quiffy remaining alive against a wall or ceiling after his
Life Force reaches zero. A dead airborne Quiffy now falls automatically until
he reaches south support, regardless of held movement input or the attachment
selected immediately before death.

## Instruction-level correction

The original path at `$DBE8-$DC28` treats death mode 1 specially before the
ordinary movement selector:

- if contact bit `$10` (south support) is set, it moves Quiffy up eight pixels,
  starts death mode 2, and requests sound `$38`;
- otherwise it clears the complete contact word at `$17E68` and continues at
  `$DC2C`.

Clearing that word is the important detail. It discards wall, ceiling, corner,
and attachment contacts for movement selection, forcing the no-contact gravity
path. The previous host retained the reconstructed contact mask, so held input
could continue surface movement and suspend the death until the player released
the control.

The host now performs this branch in the original position, after contact
maintenance/resolution and before movement. This also means a grounded death
enters the cross sequence before ordinary movement, rather than moving once and
then landing afterward.

## Verification

- held up/right input against an east wall: contact word clears and vertical
  velocity becomes positive on the first death update;
- repeated held input: Quiffy continues descending without player release;
- ordinary south-support death lifecycle and `$78-$7F` cross cadence: pass;
- strict C99 build with warnings as errors: pass;
- complete core suite under UndefinedBehaviorSanitizer: pass.

Artifact: `airborne_death_fall_milestone88.png`.
