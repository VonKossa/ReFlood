# ReFlood Milestone 156: experimental local multiplayer

## Scope

The Settings screen now offers independent Player 1 and Player 2 input assignments and **Start Experimental Multiplayer**. The choices are Arrows / Right Ctrl, WASD / Left Ctrl, or either of the first two connected controllers. Names are displayed for connected controllers, and the menu requires two distinct connected choices. The original saved single-player input setting remains separate.

The experimental host uses a 640×360 logical 16:9 stage with a vertical divider. Each side presents a 320×208 gameplay view scaled proportionally, plus player identity, controls, health, air, lives, and the shared trash count. Quiffy 2 and their Matilda use related green/blue palette variants at render time; no extracted sprite assets are modified.

## Shared simulation

`src/multiplayer_game.inc` owns the second actor's movement, item, weapon, camera, HUD, death, and Matilda state. The original `flood_game_tick` advances the shared cavern, water, and object dispatcher once per frame. The second actor then uses the original private player and item routines against that same world. The actor's state is switched back before the next frame. Each actor has a separate lives count; the survivor becomes the authoritative actor if the first runs out of lives.

The players spawn at the same marker with a short collision-checked horizontal offset. Trash and score are shared. Once the trash count reaches zero, either player's exit contact completes the cavern. Weapon collision regions can damage the opposing player. Each Matilda follows and damages its own player.

The mode does not use shipped-level coordinate exceptions, preserving a path to future custom levels. It is explicitly opt-in; the single-player gameplay loop is still used for Start Game.

## Verification

- Strict C99 core suite: pass with independently extracted, manifest-verified data.
- SDL input/settings stub suite: pass, including experimental host initialization and two-panel render smoke.
- Real SDL2 2.30 headers and the installed SDL2 runtime: strict GCC build passes. With SDL's dummy video and audio backends, an offscreen software renderer produced two populated gameplay panels and a centered divider; the captured frame was inspected for layout and Player 2's palette. A multiplayer session started and exited cleanly after a queued SDL quit event.
- New multiplayer core suite: pass. It covers independent actor and Matilda histories, one shared clock step, shared exit completion by Player 2, friendly fire from each player, survival after Player 1 is out, and one multiplayer tick on all 42 shipped levels.
- UndefinedBehaviorSanitizer core and multiplayer suites: pass.
- Strict headless original-mode build and all 42 level smoke runs: pass.
- Release data directory: empty. No extracted Flood assets, executable, object, or build directory in the source package.

## Experimental limits

The original enemy dispatcher still targets the authoritative actor. The second actor receives a separate contact check against the resulting enemy positions, rather than every original enemy-specific interaction. A physical window, GPU backend, and real controller hot-plug could not be tested in this headless environment; the SDL test used its software renderer and dummy video/audio backends. CMake is unavailable, so the real SDL build used strict GCC directly. The experimental mode starts directly at the requested level and does not yet use the single-player intro, protection, ending, or high-score presentation sequences.
