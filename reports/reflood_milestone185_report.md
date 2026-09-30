# ReFlood Milestone 185: custom sequence completion screen

Completing the final level in a custom sequence now displays **CONGRATULATIONS** in the white Flood font before returning to Settings. It uses the same centered vertical position as multiplayer's **GAME OVER** text. In multiplayer, the message appears on both player views; in single-player, it appears over the final level's screen. The display lasts three seconds, then the normal fade and pending exit audio finish.

The screen also appears after a standalone custom map. Numbered sequences continue normally while the next numbered header exists. The original campaign ending and high-score flow are unchanged. The README now describes the final screen and its duration.

## Verification

- Built the SDL2 game with `-Wall -Wextra -Wpedantic` without warnings.
- The SDL input and multiplayer host tests passed using locally extracted original data. A visible desktop playthrough was unavailable.

This ZIP includes reports through milestone 185, contains no extracted game data, and retains an empty `data/` directory with mode 755. ZIP timestamps are normalized to 2025-01-01.
