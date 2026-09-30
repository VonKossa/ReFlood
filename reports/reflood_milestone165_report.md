# ReFlood Milestone 165: multiplayer death, pause, and restart controls

## Changes

- Once a player has no lives, their Matilda is removed from the simulation's visible state and both split-screen render paths. The player's own death-cross animation still completes before `GAME OVER` appears.
- Escape now starts the original falling and cross death sequence for both active players. The shared game stays alive until both animations finish, then uses the full-window shared high-score path from milestone 164.
- P pauses the shared simulation. Either assigned player's Fire resumes it after the same press-then-release wait used by single player. A held Fire at pause time requires only its release.
- R uses the original sequence: fade, reset the current cavern, complete the 51-frame level banner, then charge one life to each surviving player. A last-life actor completes the original death animation; the other player continues if they still have lives. Scores remain separate and persist through restart.

## Verification

- Strict C99 core and multiplayer suites passed. New checks cover simultaneous Escape death animations, restart timing/life deductions, score preservation, and a surviving player after the other player's last life.
- SDL rendering/input stub suite passed. New checks cover Matilda disappearance, death-cross versus `GAME OVER` timing, pause resumed by Player 2's Fire (including initially held Fire), and Esc advancing to the shared score path rather than closing the host.
- Strict real SDL2 build and UndefinedBehaviorSanitizer SDL suite passed.
- The source ZIP includes an empty `data/` with mode 755 and fixed 2025-01-01 entry timestamps.

Input and rendering were verified with SDL stubs and a real SDL2 build, not manual physical-controller play. The final-level ending remains the existing experimental-mode limitation.
