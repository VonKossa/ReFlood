# ReFlood Milestone 160: unified Player 1 input and level handoff

## Changes

- Removed the separate Input row from Settings. Player 1's assignment controls single-player gameplay and the presentation screens after Settings; Player 2 remains independent for experimental multiplayer. Arrows / Right Ctrl, WASD / Left Ctrl, and the first or second connected controller work for Player 1. Saving writes `player1=` and `player2=`; existing `input=keyboard/gamepad` files are read as a Player 1 migration fallback. A new choice also applies when Start Game is selected without saving.
- Resizing a windowed stage now calls SDL's centered window position, including the switch to the 640×360 multiplayer stage.
- Both actors receive the first-frame player/protection update during the original 51-frame, 60 ms per frame level banner. The shared clock advances once per frame, and the level-start protection sound is deferred until active play. This applies at initial entry, subsequent levels, and restart. Both actors enter with protection and blinking; restart deducts surviving players' lives after the banner as in the single-player sequence.
- Completion now fades both rendered views and HUDs through sixteen 20 ms steps, waits for exit audio to finish, loads the next cavern, resets Player 2 while preserving their score and lives, and plays the full banner before gameplay resumes. The frame deadline restarts after each blocking transition.

## Verification

- Strict C99 core and multiplayer suites: pass; multiplayer checks that both actors' protection timers go from 100 to 99 on the first banner frame while the shared clock advances exactly 51 frames.
- SDL input/settings stub suite: pass, including the eight-row menu, P1 WASD and second-controller routing, legacy configuration migration, saved P1/P2 settings, window recentering, and a full multiplayer entry banner.
- Strict real SDL2 GCC build and dummy video/audio multiplayer session: pass.
- A scratch-only synthetic level with a nearby exit and no trash reached completion just after the entry banner. A real SDL software-rendered host then faded, waited for exit audio, advanced to level 2 at approximately 5.8 seconds, completed the second banner, and continued running. The synthetic data is absent from the release.
- UndefinedBehaviorSanitizer core and multiplayer suites: pass. The source package has an empty `data/` directory and no extracted game assets or compiled binaries.

## Experimental limits

The multiplayer route still starts directly at the chosen level and does not include single-player intro, copy protection, the level 42 ending, or high-score presentation. Zap messages remain simplified in experimental mode. Enemy targeting still follows the shared dispatcher and the separate second-player contact check. Hardware controllers, a GPU renderer, and a physical display were unavailable in this headless environment.
