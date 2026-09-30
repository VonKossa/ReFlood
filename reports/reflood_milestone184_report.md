# ReFlood Milestone 184: numbered custom-map sequences

## Changes

- Custom maps named `level_01_header.bin` through `level_99_header.bin` now advance to the immediately following numbered header in the same directory after exit. Selecting a later number starts there. A missing next header ends the sequence, so numbering gaps stop progression. Arbitrary names remain standalone maps.
- The same progression runs in single-player and split-screen multiplayer. The current custom header is retained for restart; scores and remaining lives carry into the next custom level. Custom sequences return to Settings at their end, without entering the original ending or high-score flow.
- The custom loader now preserves numbers 43–99 for the level banner instead of displaying them as level 1.
- The README describes the four-file naming pattern, start-at-selection rule, gap handling, and standalone names.

## Verification

- Built the SDL2 game with `-Wall -Wextra -Wpedantic` without warnings.
- Core and SDL input tests passed using locally extracted data. Sequence tests cover next-header detection, a missing number, a standalone name, level 99, and path capacity.
- A local level-43 file set based on extracted test data loaded with banner number 43. No extracted game data is included in the package.

The ZIP's empty `data/` directory has mode 755 and ZIP timestamps are normalized to 2025-01-01. A visible desktop playthrough was unavailable.
