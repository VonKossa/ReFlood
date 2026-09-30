# ReFlood Milestone 174: fullscreen grid lines

The missing grid lines were specific to fullscreen scaling. A one-pixel line drawn on the editor's 1280×840 logical canvas can be lost when SDL scales that canvas to the display. In fullscreen, the editor now draws grid boundaries as one-pixel lines directly in the output resolution after scaling the map. Windowed grid rendering remains in logical coordinates. The canvas clip and SDL logical size are restored before the palette and other UI elements render.

## Verification

- Compiled the SDL editor and UI interaction test with `-Wall -Wextra -Wpedantic`.
- SDL dummy renderer pixel checks confirmed the 10th and 11th vertical and horizontal grid lines at logical size, at scaled 1920×1080 and 1366×768 output sizes, and after an actual SDL fullscreen transition.
- Editor interaction tests passed. A visible desktop/GPU session and CMake were unavailable in this workspace.

The package contains no extracted copyrighted data. Its `data/` directory is empty with mode 755, and ZIP timestamps are normalized to 2025-01-01.
