# ReFlood Milestone 166: weapon friendly fire and level-42 ending

## Changes

- Dynamite's first blast frame now applies the original 40 Life Force contact damage to the other player as it does to its owner, including during protection. Grenade blast phases likewise mirror their original 10-point overlap checks for the other player. Their object-hit calls still update enemies and score, but no longer add the generic 16-point PvP hit on top of those blast rules.
- Boomerang, shuriken, flamethrower, and radial flame keep the existing generic multiplayer collision rule. The ordinary single-player damage and action behavior is unchanged.
- Completing level 42 performs the existing split-screen exit fade followed by the original second ending fade. The original 75-event ending then runs in a single full-width window at 20 ms per frame, followed by its fade and the shared high-score/title sequence. Each qualifying player's name-entry turn remains available.

## Verification

- Strict C99 original core and multiplayer suites passed. Level-2 tests cover dynamite in both attack directions under protection, grenade overlap, the other projectile/beam families, and the level-42 completion state.
- SDL presentation stub suite passed; the ending ran all events at the original cadence and final fade on one window. Earlier startup, pause, Esc, restart, and dual-score tests still pass.
- Strict real SDL2 build and UndefinedBehaviorSanitizer SDL suite passed.
- The source ZIP includes an empty `data/` with mode 755 and fixed 2025-01-01 entry timestamps.

The ending and combat were checked through simulation and SDL stubs plus a real SDL2 build; manual play on a physical display/controller was not performed. Copyrighted extracted data is not in the package.
