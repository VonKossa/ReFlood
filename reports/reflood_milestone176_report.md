# ReFlood Milestone 176: fullscreen editor text

Fullscreen glyphs are now positioned and drawn in output pixels at an integer font scale. SDL's fractional logical-canvas scaling no longer samples individual text strokes. Windowed labels continue through the existing glyph texture and logical renderer. The fullscreen text pass runs after the map, palette, and dialogs, then restores the logical canvas for the next frame.

This addresses the remaining real-display clipping reported after milestone 175. The `B: PALETTE BANK` label and two-column control layout remain unchanged.

## Verification

- Compiled the SDL editor and interaction test with `-Wall -Wextra -Wpedantic`.
- Pixel checks passed for a continuous top stroke of the `B` glyph in the physical fullscreen text pass at simulated 1920×1080 and 1366×768 outputs. Existing logical text, colon, grid, browser, and shortcut tests passed.
- Inspected a fullscreen sidebar render at 1920×1080: all labels are complete and the two columns remain separate.
- A visible GPU desktop was unavailable; this fix requires confirmation on the user's fullscreen display. CMake was unavailable in this workspace.

The ZIP includes no extracted copyrighted data. Its empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
