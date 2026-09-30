# ReFlood Milestone 175: palette-bank label and fullscreen font

The editor sidebar now reads `B: PALETTE BANK`.

The extracted 8×8 font was previously drawn as individual SDL points. Fractional fullscreen scaling placed those points sparsely, making letters look clipped or like other letters. The editor now builds a transparent glyph texture from the same extracted font and renders whole glyphs through SDL. The colon keeps its corrected dot shape. This changes only editor text rendering; the original game presentation is untouched.

## Verification

- Compiled the editor and SDL interaction test with `-Wall -Wextra -Wpedantic`.
- SDL dummy-renderer checks passed at 1920×1080 and 1366×768. The top stroke of the `B` glyph remains a continuous run of output pixels after scaling; the existing colon and grid checks pass.
- Inspected a rendered fullscreen sidebar screenshot: the `B: PALETTE BANK` label and adjacent controls are legible.
- A visible desktop/GPU session and CMake were unavailable in this workspace.

The source ZIP contains an empty `data/` directory with mode 755, no extracted copyrighted assets, and normalized 2025-01-01 timestamps.
