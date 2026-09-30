# ReFlood — Milestone 125

## Direction-aware corner attachment

Milestone 124 restored responsive ordinary movement but released an attachment
for every nonzero perpendicular input. That also removed the intended corner
assist: while climbing the left side of a block, for example, holding Up and
adding Right points into the block and must remain attached until the upper
diagonal probe becomes the active contact.

## Correction

Perpendicular input is now compared with the cached surface normal:

- input pointing into the attached surface retains the attachment and permits
  the upcoming corner handoff;
- input pointing away from the surface releases immediately;
- tangent input continues to refresh from the current keyboard state;
- the zero-contact gap and lone-diagonal handoff behavior from milestone 124
  remain in place;
- the ten-update safety timeout is unchanged.

At the diagonal corner probe, the tangent is zeroed and the retained normal is
replayed. Quiffy therefore takes the horizontal or vertical step around the
corner without requiring exact pixel alignment. Once the new cardinal surface
is established, ordinary live input resumes.

## Verification

- strict C99 core build with all warnings treated as errors: pass;
- complete gameplay and presentation regression suite: pass;
- SDL input/settings regression suite using the SDL stub: pass;
- inward versus outward attachment input for ceiling and wall states: pass;
- two-contact wall climb retains an inward diagonal command: pass;
- lone-diagonal wall-to-top turn produces the automatic horizontal step: pass;
- integrated Up-then-Up+Right block climb completes the corner step and full
  top jump: pass;
- cardinal-surface immediate movement and release regressions: pass;
- jump-and-fire and falling-flamethrower regressions: pass;
- 300-update headless Level 1 smoke test: pass.
