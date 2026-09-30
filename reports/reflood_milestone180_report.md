# ReFlood Milestone 180: clearer marker descriptions

The editor's T: Tiles reference now explains tiles 001–006, 010, 011, and 019 using their verified level initialization behavior. It names Doctor Dusty, the probable Snail, Beady Ball, and the Mine; explains that tile 003 is a trigger switch whose graphic is hidden at level start; and distinguishes cleared markers from the dormant mechanism that becomes tile 203. Tile 019 becomes tile 230 after it spawns the Mine.

The reference still previews the original bank tile graphics, which are marker symbols rather than the creatures' runtime sprites. No game mechanics or level file format changed.

## Verification

- Compiled the SDL2 editor and passed its existing UI test with `-Wall -Wextra -Wpedantic`.
- Inspected the changed descriptions in a 1920×1080 SDL dummy render. Replaced semicolons with visible hyphens in the Flood font and confirmed the text fits the popup.

The ZIP includes no extracted copyrighted data. Its empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
