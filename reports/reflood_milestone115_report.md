# ReFlood — Milestone 115

## Scaler setting

The startup menu now contains a pending `SCALER` row with three choices:

- `NEAREST`: explicit nearest-pixel sampling and the default/current look;
- `LINEAR`: SDL linear texture filtering;
- `SCALE2X`: an independent edge-aware 2× ARGB software scaler.

Changing the row has no immediate effect. `SAVE SETTINGS` writes the selected
scaler with the display mode to `./config/configuration`; `START GAME` ignores
unsaved changes. Older configuration files without `scaler=` remain valid and
default to Nearest.

## Unified output path

Every native framebuffer now passes through one scaler-aware texture upload
path. This covers gameplay, fades, intro, title, level selector, copy
protection, zap panels, high scores, and ending. Scale2X uses a dynamically
sized intermediate buffer (at most 640×480 for current content), while Nearest
and Linear upload native pixels directly. Stage and fullscreen logical sizes
track the scaler's output factor without changing game timing.

## Menu colon correction

The copy-protection font stores a solid block at the ASCII colon position. The
settings renderer now draws a dedicated two-dot colon, eliminating the white
square while retaining the original font for all other characters.

## Verification

- strict C99 core and SDL-stub builds with warnings as errors: pass;
- original gameplay and presentation regression tests: pass;
- configuration parsing and round trip for all three scaler names: pass;
- pending scaler changes and explicit Save/Start actions: pass;
- exact Scale2X 3×3-to-6×6 neighbourhood output: pass;
- native versus doubled texture dimensions and logical stage sizes: pass;
- explicit SDL Nearest and Linear texture modes: pass;
- colon raster contains two dots rather than the font's 8×8 block: pass;
- all direct texture updates outside the shared output helper: none.
