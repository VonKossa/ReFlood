# Flood Milestone 43 — runtime State 22 bolt launcher

## Initializer

Marker 20 dispatches through `$EAA4` and creates runtime state `$16` (22). The
initializer copies current X/Y into record words +4/+6 as the saved emitter
origin, clears impact byte +8, and stores sprite base `$14` at +14. The SDL
loader now performs the same conversion and clears the marker tile.

## Live update (`$1363A`)

When impact byte +8 is clear, the handler:

1. sweeps a 32x8 rectangle from `(x,y+4)` by `(dx=8,dy=0)`;
2. enters impact mode if the swept X flags contain solid bit 1;
3. otherwise draws sprite `$14` at `(x+16,y)` and sprite `$16` at `(x,y)` for
   the normal one-unit game-speed case;
4. tests `(x,y+4,32,8)` against Quiffy and writes Life Force -1 on overlap;
5. advances `x` by 8, even on a player hit;
6. always draws emitter sprite `$17` at the saved origin.

Terrain impact branches before player testing and movement.

## Impact and reset (`$13718`)

Impact phase byte +9 advances from 0 through 7. The six visible updates select
`$98,$99,$99,$9A,$9A,$9B`. Phase 7 clears impact mode and phase state, then
restores current X/Y exactly from the saved origin. Sound calls are identified
but remain outside the deterministic host core.

## Integration and validation

State 22 objects have explicit origin, sprite-base, and state-flag fields. The
SDL host composites both live bolt segments, the stationary emitter, and impact
frames. Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` passes.
Tests cover swept terrain impact, all six visible frames, seventh-update reset,
instant player death with final advance, and Level 9's two static marker-20
initializers.

## Remaining boundary

Static and initially loaded State 22 traps are integrated. Trigger rectangles
that dynamically paint marker 20 still need the general trigger dispatcher to
invoke the same marker initializer at runtime. The next reverse-engineering
target can therefore be either that trigger path (`$12DA6/$12E12/$12E48`) or
the unresolved static hazard/Sparkling Fungi search.
