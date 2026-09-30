# ReFlood Milestone 178: editor marker labels

The two editor sidebar reminders now read “Tile 22: Start position” and “Tile 23: Camera bounds.” This clarifies that the numbers identify palette tiles and that the bounds marker controls the camera.

## Verification

- Compiled the SDL2 editor with `-Wall -Wextra -Wpedantic`.
- Inspected a 1920×1080 SDL dummy render of the fullscreen sidebar. Both labels render completely within the left control column using the Flood font, with no overlap into the shortcut column.

The ZIP includes no extracted copyrighted data. Its empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
