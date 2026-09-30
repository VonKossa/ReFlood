# ReFlood — Milestone 112

## Startup settings

ReFlood now opens a settings screen as its first interactive state. No cavern,
music, title, or intro content is initialized until the player selects
`START GAME`.

The screen uses the original `presentation/LEVEL_font.bin` glyphs from the
copy-protection section, drawn as white text on an opaque black 320×208 canvas.
It contains:

- `SETTINGS` at the top;
- a `DISPLAY: WINDOWED` / `DISPLAY: FULLSCREEN` toggle;
- `START GAME`.

Up/Down changes the selected row. Left/Right or Right Ctrl/Return changes the
display mode on the Display row; Right Ctrl/Return starts the game on the Start
row. Held inputs are edge-latched to prevent repeated changes.

## Configuration

The current display mode is stored in `./config/configuration` as
`fullscreen=0` or `fullscreen=1`. Missing or unrecognized configuration
defaults safely to windowed mode. Writes use a temporary file followed by a
rename, and the `./config` directory is created when necessary. The existing
in-game `F` fullscreen toggle updates the same file.

## Verification

- strict C99 core compilation and tests: pass;
- strict C99 SDL input-stub compilation and tests: pass;
- configuration default, parsing, persistence, and invalid-value tests: pass;
- settings navigation, toggle latching, start action, font loading, and
  black/white framebuffer tests: pass;
- source-order check confirms the settings loop precedes level, music, and
  intro loading;
- release cleanup check finds no `build-*` directories or `cc*.o` files.
