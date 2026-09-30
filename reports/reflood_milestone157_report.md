# ReFlood Milestone 157: multiplayer HUD and individual scores

## Changes

- The center divider is one logical pixel wide and black. The 640×360 stage remains a 16:9 vertical split; each gameplay view is scaled into its side with a dedicated header and footer.
- Quiffy 2 and Matilda 2 now use the same green palette mapping for masked sprite pixels. Original sprite files and the first player's palette are untouched.
- Each player's footer shows their own score, lives, health, and air. Health and air also have color-coded gauges. Both footers read the shared cavern's remaining trash count.
- Score is part of the private multiplayer actor state, so pickups and weapon points belong to the actor who earns them. Player 2's score and lives persist through level changes; a fresh multiplayer session resets their score. The single-player path remains unchanged.
- The multiplayer HUD and simulation remain in the dedicated `src/multiplayer_host.inc` and `src/multiplayer_game.inc` files, with minimal existing render hooks. The shared world model has no shipped-level coordinate assumptions, preserving the path to custom levels.

## Verification

- Strict C99 core and multiplayer tests: pass. The multiplayer test checks that either player's pickup decreases one shared trash counter while only that player's score increases, and that Player 2's score survives a reset. It also covers exit completion, friendly fire, survivor play, and one frame on all 42 extracted levels.
- UndefinedBehaviorSanitizer core and multiplayer tests: pass.
- SDL input/settings stub test: pass.
- Strict GCC build against real SDL2 headers/runtime: pass. SDL dummy video/audio with a software renderer: both gameplay panels populated, divider pixel black, separate rendered scores (1234 and 5678), shared trash (17), and a clean multiplayer session start/exit. The captured frame was inspected visually.
- The source package has an empty `data/` directory and contains no extracted game data or compiled binaries.

## Experimental limits

The second player still receives a separate enemy contact check after the shared enemy dispatcher, which targets the authoritative player. Hardware controller hot-plug, a physical display, and GPU backends were not available in this environment. The experimental mode starts directly at a level and does not yet include the single-player ending and high-score presentation.
