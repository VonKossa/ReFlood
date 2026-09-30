# ReFlood — Milestone 122

## Why milestone 121 looked unchanged

Milestone 121 preserved the original attachment state at a lone diagonal
contact. That handoff occurs only after Quiffy has already reached exact corner
alignment, so it did not restore the original earlier turn visible during the
final approach.

The remaining loss was in the final movement collision resolver. For a
diagonal command it tested the horizontal edge at the old vertical position
and the vertical edge at the old horizontal position. Near a convex corner,
one old edge still touches the surface even though applying both four-pixel
components produces a completely clear destination. ReFlood discarded that
component, reached alignment on one update, and did not turn until the next.

## Correction

When both direction inputs request diagonal movement around a corner:

- the ordinary swept collision query still runs first;
- if one axis is blocked, that axis is checked again at the position produced
  by the other axis;
- when the shifted sweep is clear, both four-pixel components complete on the
  same update;
- if the diagonal destination remains occupied, the normal collision block is
  retained.

The extra check requires live input on both axes. Synthetic surface-normal
motion therefore cannot consume the Up-only wall-top jump fixed in milestone
118.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete gameplay and presentation regression suites: pass;
- all eight wall, floor, and ceiling convex-corner approaches turn one
  four-pixel update before exact alignment: pass;
- the combined destination advances four pixels on both axes: pass;
- blocked diagonal destinations remain subject to ordinary collision: pass;
- milestone 120 immediate cardinal-surface response: pass;
- milestone 121 lone-diagonal handoff: pass;
- milestone 118 Up-only wall-top full jumps on both sides: pass;
- jump-and-fire and falling flamethrower regressions: pass;
- 300-update headless Level 1 smoke test: pass.
