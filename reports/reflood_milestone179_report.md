# ReFlood Milestone 179: tile reference popup

The editor sidebar now shows `T: TILES` beneath `B: PALETTE BANK`. Pressing T opens an in-editor reference with all tile IDs from 000 through 255 in numeric order. It shows each tile's current bank graphic and description. The old sidebar start and bounds reminders are in the reference as `022 : Player start position` and `023 : Camera bounds`.

The popup has descriptions grounded in ReFlood's runtime handling for markers, water, trash, the exit, transport pairs, pickups, weapons, and triggers. Other entries describe bank-specific collision attributes read from the extracted `BLOCK*_attrs.bin` file; when attributes are unavailable, they say so. Unidentified artwork is left as a bank graphic without a speculative name.

Use Up/Down, Page Up/Page Down, Home/End, or the mouse wheel to browse. Clicking a row selects that tile and closes the reference. T, Esc, or the Close button closes it without selecting a tile. The editor README documents these controls.

## Verification

- Compiled the SDL2 editor and UI test with `-Wall -Wextra -Wpedantic`.
- UI tests pass for all 256 nonempty descriptions, the exact start and bounds meanings, scrolling limits, modal close, and the preexisting editor interactions. Level round trips also pass.
- Inspected a 1920×1080 SDL dummy render of the popup and its 021–025 page. The labels and graphics fit the modal without clipping. A visible desktop was unavailable.

The ZIP includes no extracted copyrighted data. Its empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
