# Flood Milestone 48 — complete runtime State 14

The object dispatcher at `$CC60` selects `$10C46` for runtime state 14,
Plonkin Donkin. Marker 14 enters through `$E974`: `$EC0A` records the marker
position and state, then the initializer writes horizontal velocity `+4`,
vertical velocity `+16`, clears the map marker, and advances the object slot.

## Exact update path

Each update copies the object's position and velocity to local words. Record
word `+8` selects the acceleration phase: zero subtracts one from vertical
velocity, while nonzero adds one. Downward velocity is capped at `+16`; an
upward value below `-12` is clamped and changes `+8` to one.

`$F0E0` performs a swept 16×16 collision query. Solid X-edge contact reverses
the local horizontal velocity. A solid Y edge reverses upward motion directly.
For downward motion it halves the speed; results of two or less become 12,
then the value is negated to make the rebound upward. The downward collision
also changes record `+8` to one.

Before committing motion, `$F50A` samples the attribute of the current centre
cell `(x+8,y+8)`. A solid centre suppresses the entire position and velocity
commit for that update. Animation and player contact still proceed.

## Rendering and contact

Animation uses the alternating `$17E86` step and wraps through four frames,
`$DE-$E1`. `$E4AA` draws at `(x-6,y-14)`. `$10DCA` tests Plonkin's 16×16
body against Quiffy's centered 16×16 body and subtracts 8 from Life Force.

The SDL renderer now includes State-14 objects at the recovered registration
coordinates. Tests cover free motion, the upward clamp and phase switch,
minimum and ordinary floor rebounds, horizontal reversal, the solid-centre
commit gate, animation, draw coordinates, damage, and all-level population.
