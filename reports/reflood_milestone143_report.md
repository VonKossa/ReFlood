# ReFlood — Milestone 143

## Confirmed Sparkling Fungi identity

Milestone 143 records the definitive identification of Sparkling Fungi without
changing gameplay behavior.

The recognizable object immediately left/below Quiffy's Level 23 start is
BLOCKA tile `$3D` at map coordinate `(1,9)`. Quiffy starts at `(4,5)`.
Tile `$3D` carries terrain attribute bit 6 (`$40`), already named
`FLOOD_ATTR_FATAL`; touching it therefore enters the ordinary instant-death
path exactly as the original does.

Although it appears as an object in the artwork, it is represented internally
as static terrain rather than as one of the 25 runtime object states.

## Historical correction

This resolves the long-running naming uncertainty:

- the early provisional “State 22 = Sparkling Fungi” theory remains withdrawn;
- runtime State 22 is correctly identified as the horizontal bolt/rocket
  launcher;
- Milestone 57 correctly recovered the general fatal-terrain mechanism, but its
  suggestion that BLOCKB tile `$47` was the clearest Fungi candidate was not
  the Level 23 object;
- the confirmed Level 23 Sparkling Fungi is BLOCKA tile `$3D`.

The same BLOCKA `$3D` tile is shipped once each in Levels 6, 23, and 28. Other
tiles also use `FLOOD_ATTR_FATAL` for different lethal artwork, so the engine
continues to keep the behavior named generically rather than special-casing a
single tile number.

## Verification

A shipped-map regression now loads Level 23 and verifies:

- Quiffy's start marker resolves to tile coordinate `(4,5)`;
- the identified Fungi cell is `(1,9)`;
- that cell contains BLOCKA tile `$3D`;
- tile `$3D` carries `FLOOD_ATTR_FATAL`.

No executable behavior changed. The package remains source-only.

