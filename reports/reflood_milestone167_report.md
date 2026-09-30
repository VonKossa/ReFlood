# ReFlood Milestone 167: weapon-hit feedback for both players

## Changes

- Cross-player weapon hits now feed the damaged actor's original hurt-feedback latch even when they occur before or after that actor's native damage-feedback pass. The original hurt bubble appears in both views, and a hurt voice plays once on contact entry rather than repeating during continuous damage. A clear frame resets the latch so a new hit can play the voice again.
- Player 1's hit on protected Player 2 is preserved after Player 2's protection refresh, matching the reverse attack order. Dynamite's original 40-point hit remains reflected in the Life Force gauge and hurt feedback.
- The adjustment is in multiplayer simulation code; single-player damage handling and rendering remain unchanged.

## Verification

- Strict C99 core and multiplayer suites passed. A level-2 regression checks dynamite, grenade, boomerang, shuriken, flamethrower, and radial flame in both attack directions, with Life Force loss, hurt bubble, and original voice IDs. Dynamite is also checked under protection, through sustained contact, separation, and recontact.
- SDL input/presentation stub suite and strict real SDL2 build passed. The existing pixel check confirms the remote actor's hurt bubble appears in the observer's view.
- UndefinedBehaviorSanitizer multiplayer suite passed.
- Source ZIP includes an empty `data/` directory with mode 755 and fixed 2025-01-01 entry timestamps.

The feedback was checked in the simulation sound queue and SDL pixel buffers, not listened to on physical speakers. Copyrighted extracted data is not included.
