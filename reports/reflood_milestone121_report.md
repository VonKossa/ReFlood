# ReFlood — Milestone 121

## Original corner assistance

Milestone 120 removed the remaining direction-change delay by releasing a
surface attachment whenever the perpendicular input axis became active. That
was correct on an ordinary cardinal surface, but too broad at a convex corner.

The reconstructed original paths already distinguish this case:

- `$D99E-$DAEA` uses missing adjacent diagonal support and travel direction to
  select how Quiffy rounds a corner;
- `$DAEC-$DBE6` retains the previous surface normal during a short attachment
  handoff;
- a lone diagonal collision probe identifies the active corner transition.

When two directions were held at that point, milestone 120 discarded the
handoff. Raw diagonal movement then reached the final collision check, which
cancelled one component and forced pixel-perfect manual alignment.

## Correction

Immediate attachment release is now geometry-sensitive:

- a perpendicular command on a cardinal floor, ceiling, or wall releases the
  attachment immediately;
- a lone diagonal contact preserves the existing attachment mode;
- during that handoff the tangent component is suppressed and the retained
  surface normal supplies a full four-pixel step around the corner;
- the next cardinal probe can then establish the new surface normally.

This separates stale input state from the deliberate original corner-handoff
state instead of treating both as unwanted cache.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete gameplay and presentation regression suites: pass;
- all eight ceiling, floor, east-wall, and west-wall corner orientations retain
  their handoff with both direction inputs held: pass;
- an integrated lone-diagonal transition moves four pixels around the corner
  without exact alignment: pass;
- milestone 120 immediate cardinal-surface direction changes: pass;
- milestone 118 wall-top full jumps on both sides: pass;
- jump-and-fire and falling flamethrower regressions: pass;
- 300-update headless Level 1 smoke test: pass.
