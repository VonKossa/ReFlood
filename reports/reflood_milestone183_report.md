# ReFlood Milestone 183: choose a map, then choose a mode

## Changes

- **Choose Custom Map** now stores the selected header in Settings. It no longer starts the game immediately. The selected filename appears at the bottom of Settings.
- **Start Singleplayer Game** and **Start Multiplayer (Experimental)** both use that custom map when selected. With no selection, each follows its existing original-game path.
- Custom multiplayer uses the split-screen host, including its shared world, separate players and scores, level banner, restart, and pause behavior. Restart reloads the selected custom header. Completing the custom level or exhausting both players returns to Settings without advancing the original campaign or entering its high-score flow.
- Esc in the browser keeps the current selection. Delete or gamepad X clears it, restoring the original game for either start option. The selection stays available when returning to Settings after a custom session.
- Updated the README and SDL input tests for the revised flow.

## Verification

- Built the SDL2 game with `-Wall -Wextra -Wpedantic` without warnings.
- Core, multiplayer core, and SDL input tests passed using locally extracted data. SDL input coverage includes preserved selection for both start entries, browser clearing, and a selected header running through the split-screen host.

The source ZIP contains no extracted copyrighted data. Its empty `data/` directory has mode 755 and ZIP timestamps are normalized to 2025-01-01. A visible desktop playthrough was unavailable.
