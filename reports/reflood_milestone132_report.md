# ReFlood — Milestone 132

## Selectable gamepad input

The startup settings menu now includes an `INPUT` option with `KEYBOARD` and
`GAMEPAD` choices. Like the existing display, scaler, and game-speed options,
the pending choice takes effect only when `SAVE SETTINGS` is selected. It is
stored in `./config/configuration` as either `input=keyboard` or
`input=gamepad`. Missing or older configuration files remain compatible and
default to Keyboard.

Gameplay gamepad mapping:

- D-pad or left stick: movement;
- A: fire;
- Back: Escape-equivalent abort to zero lives.

ReFlood uses SDL's game-controller mapping layer and selects the first device
SDL recognizes as a game controller. Controller insertion and removal events
are handled while the program is running. A disconnected selected controller
can therefore reconnect without restarting the game.

The startup settings screen accepts keyboard and gamepad input simultaneously,
independent of the saved gameplay choice. This keeps the menu operable when a
controller is absent and lets the player switch back to Keyboard without
editing the configuration file manually. The normal gameplay path continues
to isolate input to the selected device.

## Verification

- strict C99 core build with warnings treated as errors: pass;
- complete core regression suite: pass;
- strict C99 SDL-host build against the deterministic SDL stub: pass;
- SDL host regression suite: pass;
- configuration default, parser, save, reload, and legacy fallback: pass;
- settings-menu Input selection and pending-save behavior: pass;
- keyboard/gamepad gameplay isolation: pass;
- D-pad, left-stick dead zone, A, and Back mappings: pass;
- settings control by either device: pass;
- controller connection and removal handling: pass.

The package remains source-only; no executable is included.
