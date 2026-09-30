# ReFlood Milestone 170: editor window and load controls

The standalone ReFlood-Editor now starts with a blank level when launched without arguments. An explicit `--input DIR --level NN` still imports a level; `--new` explicitly requests blank even when `--input` is supplied.

## Changes

- Ctrl+F toggles fullscreen desktop mode with the editor's 1120×760 logical canvas scaled to fit while preserving its aspect ratio.
- Ctrl+Q asks whether to exit. The window close button and Esc use the same confirmation, and the prompt warns about unsaved changes. Y confirms; N or Esc cancels.
- Ctrl+L opens a path prompt for `levels/level_NN_header.bin`. It loads the matching tilemap and trigger payload, validates the optional trigger file, and updates the selected graphics bank. If edits are unsaved, a confirmation precedes the path prompt. Failed imports preserve the current level. Ctrl+V pastes paths.
- The sidebar and README document the new controls.
- Fixed a bank-switch failure so it leaves the current bank unchanged if its graphics are unavailable.

## Verification

- Compiled editor and shortcut test with `-Wall -Wextra -Wpedantic` against SDL2.
- Editor round-trip test passed across all 42 locally extracted levels; header-path parsing checks passed.
- SDL dummy-driver shortcut test passed for quit cancellation/confirmation, header loading, and both fullscreen transitions.
- Blank default and explicit imported-level startup both initialized under SDL's dummy video driver.
- CMake was not installed in this workspace; its targets were not executed here. Desktop interaction was not visually tested.

The source package keeps `data/` empty, mode 755, and does not include extracted game assets.
