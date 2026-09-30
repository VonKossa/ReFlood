# Flood reverse engineering - Milestone 33

## First integrated C/SDL host skeleton

Milestones 1-32 recovered many subsystems independently. Milestone 33 establishes the first shared host-side architecture so future recovered routines have one destination.

### Architectural boundary

`FloodGame` owns player, world, water, objects, Aunt Matilda, score/lives and the host tick. No Amiga absolute addresses appear in the public state model.

SDL2 is kept at the platform edge (`src/main.c`). `flood_core` contains no SDL dependency. This is intentional: reverse-engineered logic can be unit-tested deterministically and later compared frame-by-frame with emulator traces.

### First executable path

The host now performs:

`input -> player update -> collision/water -> Aunt Matilda -> death/respawn -> render`

When SDL2 is available, arrows control the debug Quiffy rectangle, R triggers restart/death, and the delayed Matilda outline becomes visible after the history buffer fills.

### Fidelity status

This milestone prioritizes architecture over pretending unfinished translations are exact. Known recovered constants are retained (Life Force 511, air 63, water speed 1, Matilda damage 10, 256-sample history, 20-update death delay). Two pieces are deliberately provisional: the host water fill interpolation and simplified rectangle collision. Both are called out in README and are next to be replaced by the exact recovered implementations.

### Validation

The project compiles under C99 with `-Wall -Wextra -Wpedantic`; CTest validates initialization, tile attributes, full-water fill, and Matilda death delay. The build works without SDL2 and automatically enables the SDL host when SDL2 development files are present.
