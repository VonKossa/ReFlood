# ReFlood Milestone 171: editor file browser and colon glyph

Ctrl+L now opens an in-editor file browser instead of a typed path. It lists subdirectories and `level_NN_header.bin` files, and loading a header imports its matching tilemap and trigger payload. The existing unsaved-change confirmation remains in place. The browser starts in `custom/levels` when available, falls back to the input or extracted data levels, and can navigate parent directories. C and D jump to custom and extracted levels. Arrow keys, Page Up/Down, wheel, Enter, double click, and a clickable parent entry are supported.

The editor text renderer now draws a dedicated colon glyph, fixing the white square seen in labels such as `CTRL+L:`.

## Verification

- Compiled editor, level tests, and SDL interaction tests with `-Wall -Wextra -Wpedantic` against SDL2.
- Browser test found and selected a header, loaded its companion files, and navigated parent directories.
- SDL dummy-driver interaction test covered browser loading, quit confirmation, fullscreen, and the colon's rendered pixels.
- The 42 extracted levels continued to pass the import/export round trip; blank editor startup under SDL's dummy video driver succeeded.
- CMake and a visible desktop session were unavailable in this workspace.

The package contains an empty `data/` directory with mode 755 and no copyrighted extracted assets. ZIP timestamps are normalized to 2025-01-01.
