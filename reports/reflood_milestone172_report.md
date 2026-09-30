# ReFlood Milestone 172: editor zoom, palette, and grid

The map canvas now offers nine zoom sizes: 4, 6, 8, 12, 16, 24, 32, 48, and 64 pixels per tile. `+` / `-` and Ctrl+mouse wheel change zoom. The 4 and 6 pixel sizes fit the full 128×100 map in the viewport; zooming preserves the central map area and clamps scrolling safely when the whole level fits. Partially visible edge tiles render clipped to the canvas.

The editor window grows to 1280×840, and its palette now displays all 256 tiles in 24×24 previews across a 400×400 panel. Click selection follows the enlarged layout. Ctrl+G toggles a light grid over map tile boundaries; the current grid state and zoom size appear in the sidebar.

## Verification

- Compiled the SDL editor and interaction test with `-Wall -Wextra -Wpedantic`.
- SDL dummy-driver test passed zooming to both limits, full-map scroll clamping, selecting tile 255, painting the bottom-right map tile at minimum zoom, and toggling the grid.
- Browser/level tests passed, including a round trip of all 42 locally extracted levels. Blank editor startup initialized under SDL's dummy driver.
- CMake and visible desktop interaction could not be run in this workspace.

The package includes no extracted copyrighted data. `data/` is empty with mode 755, and ZIP timestamps are normalized to 2025-01-01.
