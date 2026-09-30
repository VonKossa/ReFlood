# ReFlood — Milestone 120

## General direction-change delay

Further testing showed that the remaining delay was not limited to the
Up-to-Right ceiling transition. It could occur on any change from movement
along one surface axis to the perpendicular axis: Up/Down to Left/Right or
Left/Right to Up/Down.

The attachment helper correctly refreshed the tangent direction after the
milestone 117–119 corrections, but a one-contact surface transition could
still retain ownership of the previous surface. The new perpendicular command
was then ignored until the cached attachment reached its ten-update timeout.

## Correction

A deliberate perpendicular-axis command now releases the cached attachment on
the current gameplay update:

- Up or Down immediately releases a floor or ceiling attachment;
- Left or Right immediately releases a wall attachment;
- the contact resolver can establish the next surface mode later in that same
  update;
- the existing wall Y-alignment correction is preserved when leaving a wall.

Tangent reversals remain live while attached. The cardinal-versus-diagonal
corner handling from milestone 119 is unchanged.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete gameplay and presentation regression suites: pass;
- ceiling-to-Down and floor-to-Up respond on the first update: pass;
- east-wall-to-Left and west-wall-to-Right respond on the first update: pass;
- both positive and negative perpendicular commands release each attachment
  mode immediately: pass;
- milestone 118 wall-top full jumps on both sides remain exact: pass;
- milestone 119 cardinal and diagonal contact behavior remains exact: pass;
- jump-and-fire and falling flamethrower regressions remain exact: pass;
- 300-update headless Level 1 smoke test: pass.
