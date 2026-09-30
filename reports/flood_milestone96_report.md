# Flood reconstruction — Milestone 96

## Aunt Matilda backtracking speed

The original 68000 routine at `$CEE0-$CFFE` implements Aunt Matilda as a
256-record history reader rather than an independently moving actor. Each
record contains Quiffy's X, Y, and animation frame.

The verified order is:

1. `$CEE0-$CEF0` advances and wraps the write offset.
2. `$CEF0-$CEFE` clears the death hold if the writer has caught the reader.
3. `$CF06-$CF70` consumes, collides, and draws the current reader record.
4. `$CF72-$CF9A` decrements the hold or advances the reader normally.
5. `$CF9C-$CFC6` performs the 32-record catch-up check even during a hold.
6. `$CFC8-$CFFE` stores Quiffy's new position and animation at the writer.

Level initialization at `$E0A8/$E0D8` sets the writer to byte offset 0 and the
reader to byte offset 4: records 0 and 1 respectively. The former host reset
both indices to record 0, wrote after consuming the reader, and replaced the
original equality comparison with a numeric `lag > 8` guard inside the
non-held branch.

Those differences were especially visible after an ordinary death. Equal
numeric indices represented a nearly complete history lap, but the 20-update
hold converted that state into a short trail. During backtracking, Quiffy and
his earlier recorded path approach each other, making the error feel still
faster.

Milestone 96 restores the original initialization, update order, hold
cancellation, boundary-check placement, and exact eight-record stop
comparison.

## Timing

At the verified 60 ms gameplay cadence:

- the first valid Matilda sample appears on update 249 and is 248 updates old:
  14.88 seconds;
- the intentional extra reader advance occurs at each 32-record boundary:
  once every 1.92 seconds while Matilda is catching up;
- the stop comparison preserves an eight-record pointer gap, corresponding to
  a minimum displayed history age of nine updates (0.54 seconds).

## Regression coverage

- exact level-entry writer 0 / reader 1 initialization;
- writer pre-advance and first 32-record extra reader advance;
- first visible history sample on update 249;
- death-hold cancellation when the writer catches the reader;
- boundary catch-up while a hold is active;
- suppression of the extra skip at the original eight-record safety gap;
- existing death, Teddy, water, object, presentation, and audio regressions
  updated to use the original Matilda reset state.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
