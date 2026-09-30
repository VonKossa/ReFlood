# Flood Milestone 94: game-over return and Psycho Teddy death audit

## Result

Milestone 94 returns to the title/menu after the game-over high-score sequence
instead of terminating the application. It also confirms that Aunt Matilda
appearing behind a Psycho Teddy death is faithful original behavior.

## Game-over continuation

The original post-game routine calls the high-score insertion/editor at
`$9FF8`, then writes:

- 3 to the lives word;
- 1 to the selected-cavern word;
- 0 to the score longword.

It completes its short transition and returns to the front end. The host had
implemented the high-score presentation but then continued to SDL teardown.

Once the score screen is dismissed, the host now performs the same session
reset, reloads presentation music, and jumps to the title screen. The Bullfrog
publisher intro remains a one-time startup sequence. An explicit quit event on
the score screen still leaves the application.

## Psycho Teddy and Aunt Matilda

Psycho Teddy writes -1 to Quiffy's Life Force during the object-dispatch part
of a gameplay update. Quiffy's Life Force check has already run in that update,
so death mode begins on the next one. It uses the standard exhausted-Life-Force
branch, which applies a 20-update hold to Aunt Matilda's delayed history read.

The hold does not hide or reset Matilda. Her previously delayed sample remains
visible behind Quiffy while new player positions continue entering the history
ring. This is shared with drowning and ordinary damage deaths and is therefore
retained. A regression checks Teddy's -1 write, the next-update death-mode
transition, hold value 19 after its first decrement, and the visible `$D2`
Matilda frame behind Quiffy.

## Verification

- strict C99 core and SDL-interface builds with warnings as errors: pass;
- core and SDL-interface tests under UndefinedBehaviorSanitizer: pass;
- post-game reset policy and presentation-music replay: pass;
- Psycho Teddy/Matilda death-order regression: pass;
- all 42 cavern headless smoke runs: pass.
