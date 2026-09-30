# ReFlood — Milestone 124

## Corner assistance without movement lock

Milestone 123 proved that replaying the captured two-axis direction before
contact resolution restores the missing corner assistance. Applying the
original shared-direction-word behavior without adapting it to ReFlood's host
input loop, however, also allowed the old direction to override fresh keyboard
input for several 60 ms gameplay updates. This caused the reported sporadic
movement freezes and lockups.

## Correction

The attachment cache now has a narrow, geometry-defined lifetime:

- it survives the zero-contact sub-tile gap immediately around a corner;
- it is replayed only for a lone diagonal corner contact;
- an established north, south, east, or west surface uses current input;
- perpendicular input immediately releases an ordinary surface attachment;
- a zeroed corner tangent still releases on the next two-contact surface,
  preserving the full wall-top jump;
- the ten-update safety timeout remains in place.

This keeps the newly restored early corner handoff while preventing cached
directions from owning normal movement after the handoff.

## Verification

- strict C99 core build with all warnings treated as errors: pass;
- complete gameplay and presentation regression suite: pass;
- SDL input/settings regression suite using the SDL stub: pass;
- cardinal ceiling and floor movement responds on the first update: pass;
- perpendicular input releases cardinal wall and ceiling attachments: pass;
- cached direction survives the zero-contact corner gap: pass;
- integrated corner handoff remains two gameplay updates earlier than
  milestone 121: pass;
- both wall-top full-jump orientations: pass;
- jump-and-fire and falling-flamethrower regressions: pass;
- 300-update headless Level 1 smoke test: pass.
