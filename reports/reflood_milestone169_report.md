# ReFlood Milestone 169: ReFlood-Editor preview

This milestone adds an SDL2 executable named `reflood-editor` beside `reflood`. It is separate from the game source and uses a small standalone level I/O module.

## Implemented

- Imports existing extracted levels or creates a blank level with a Quiffy start marker.
- Displays the selected BLOCK bank's extracted tile graphics in a scrollable, zoomable map and palette.
- Tile paint and eyedropper, one-stroke undo, bank switching, and keyboard save.
- Validates level number, BLOCK bank, trigger count and presence of a start marker.
- Exports the four extracted-format level files to `custom/levels/` by default, separate from original game assets. Imported header and trigger payload bytes are preserved; the active `triggers.bin` is derived from the payload.
- Build integration, usage instructions, and editor round-trip test.

## Verification

- Compiled the editor with `-Wall -Wextra -Wpedantic` using staged SDL2 headers and the installed SDL2 runtime.
- Imported and exported every one of the 42 locally extracted original levels. Headers, tilemaps and full trigger payloads matched after round trip.
- Opened an imported level and a new blank level under SDL's dummy video driver.
- The editor's standalone round-trip and missing-start validation test passed.
- CMake and a visible desktop interaction were not available in this workspace; the CMake target is therefore unverified here.

## Current scope

This preview edits raw tiles while preserving existing trigger records. Trigger creation and visual editing are later work. The game does not yet offer a custom-level menu or load exported levels; that is the next integration milestone. No copyrighted extracted data is included.
