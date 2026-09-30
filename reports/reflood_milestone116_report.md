# ReFlood — Milestone 116

## Gameplay-speed setting

The startup menu now includes a pending `GAME SPEED` row:

- `NORMAL` preserves the milestone-115 active-gameplay cadence of one update
  every 60 ms;
- `TURBO` advances active gameplay every 30 ms, exactly twice as often.

The multiplier is deliberately applied only to the active cavern loop. It
affects the complete gameplay simulation consistently—player and enemy motion,
water, oxygen, hazards, weapons, pickups, and death behavior—without changing
the number of logical updates used by any mechanic.

The intro, title, PLAY LEVEL selector, copy protection, 51-step level banner,
zap-message panel, credits/high-score attract loop, fades, and ending retain
their existing timing. Audio mixing also remains tied to real elapsed time, so
music and effects retain their normal pitch and playback rate.

## Configuration behavior

Game Speed follows the same pending model as Display and Scaler. Changing the
row does not alter the running configuration. `SAVE SETTINGS` writes all three
values atomically to `./config/configuration`; `START GAME` ignores unsaved
changes. The new line is:

```ini
game_speed=normal
```

or `game_speed=turbo`. Existing configuration files without this key remain
valid and default to Normal.

## Verification

- strict C99 core and SDL-stub builds with warnings as errors: pass;
- original gameplay and presentation regression tests: pass;
- Normal and Turbo configuration parsing and round trip: pass;
- five-row menu navigation, pending toggle, Save, and Start behavior: pass;
- Normal interval 60 ms and Turbo interval 30 ms: pass;
- active cavern loop uses the selected gameplay interval: pass;
- no presentation or audio interval is routed through the speed setting: pass.
