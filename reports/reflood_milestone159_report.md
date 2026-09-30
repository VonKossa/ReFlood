# ReFlood Milestone 159: both Matildas can hurt both players

## Changes

- Each Matilda retains its original contact damage against its own player. Multiplayer now also checks the other player's 16×16 body against that ghost after both actors and both ghosts have moved. Each overlapping ghost deals 10 Life Force damage, so two ghosts can deal 20 in one frame. A ghost belonging to a player out of lives does not add cross-player damage.
- Cross-player damage updates the affected player's feedback and in-game HUD. The other player's score and shared trash count remain as in milestone 158.
- The unused areas above and below the two gameplay views are now black, matching the central black divider.

## Verification

- Strict C99 core and multiplayer tests: pass. New multiplayer cases check one cross hit from each ghost and two simultaneous hits on one player, alongside the existing pickup, shared trash, friendly fire, exit, survivor, and all 42 level checks.
- UndefinedBehaviorSanitizer multiplayer test: pass.
- SDL input/settings stub test and strict real SDL2 GCC build: pass.
- SDL dummy video/audio software render and multiplayer session: pass. The captured 1280×720 frame was inspected; pixels above and below the view and across the two-logical-pixel divider are black. A second-color Matilda eye pixel remains red.
- The source package has an empty `data/` directory and contains no extracted game data or compiled binaries.

## Experimental limits

The original shared enemy dispatcher still targets the authoritative player. The second player receives a separate enemy contact check. Hardware controllers, GPU rendering, and a physical window were unavailable in this headless environment.
