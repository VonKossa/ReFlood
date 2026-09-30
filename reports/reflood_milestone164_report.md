# ReFlood Milestone 164: full-window multiplayer presentation

## Changes

- Selecting experimental multiplayer now enters the same full-window intro-music lead, intro animation, title, level/password selector, and copy-protection screen used by single player. Player 1's assigned keyboard or controller drives these screens. The random seed resulting from copy protection carries into the loaded multiplayer game. The display switches to the existing side-by-side views for the level banner and gameplay.
- When both players have exhausted their lives, the two-view stage fades and the original full-window high-score board opens. Scores enter the shared top ten in descending order. Each qualifying player gets an independent turn with their assigned input and the original name editor; both edits are saved together. The original score/title cycle follows, and selecting play returns to the full-window level selector without repeating intro or copy protection.
- The single-player host path is unchanged. The existing multiplayer simulation and split rendering remain in their separate include files. The final-level ending remains a separate experimental-mode limitation.

## Verification

- Strict C99 original core and multiplayer core suites passed.
- SDL input/presentation stub suite passed. It checks the single-window intro and selector stages, gameplay split and banner, dual qualifying high-score entries with simulated independent controls, and persistence in the shared score table.
- UndefinedBehaviorSanitizer SDL presentation suite and strict real SDL2 build passed.
- Package has an empty `data/` directory with mode 755; ZIP entries use fixed 2025-01-01 timestamps to avoid clock-skew warnings.

The visual flow was checked through the SDL stub and a real SDL2 build, not by manual play on a physical display or controllers. Copyrighted extracted data remains outside the source package.
