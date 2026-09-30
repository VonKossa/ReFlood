# Flood Milestone 70: exact `$88` level-completion handoff

## Result

Milestone 70 connects the `$88` exit handler to the original outer gameplay
loop.  Completing a cavern now advances automatically through all 42 recovered
levels, performs the recovered between-level reset, and produces a distinct
whole-game completion state after level 42.

## Recovered control flow

The ordinary item chain at `$1003C-$1005C` accepts `$88` only when the
remaining-food word is zero.  It subtracts one, producing the signed sentinel
`-1`, and requests sound `$3C` (decimal 60).  The tile is not removed.

After the completed cavern's frame has been built, `$B206-$B22A` consumes that
sentinel.  It increments the level word, runs `$DF54`, clears runtime records,
and invokes the normal level setup routine.  The SDL host calls
`flood_game_advance_level()` after presenting the completed frame, preserving
the same visible ordering.

The setup routine at `$A3D2` refuses to load level 43.  The frame loop then
writes control value `-3` at `$B2E0-$B2F2`, returns to the outer program, and
the outer path calls the dedicated ending routine at `$15AD2`.  The host
represents that handoff with `game_complete=true` and terminates gameplay; it
does not substitute a speculative recreation of the original full-screen
ending animation.

## Between-level reset

The translation of `$DF54-$E10C` and `$A5A4` now restores Life Force to 511,
air to 63, and Cocktail protection to 100.  It clears Quiffy motion and death
state, carried weapon state, persistent pickup timers, the `$DC` auxiliary
record, transport state, Matilda and her history, all eight action records,
and all 128 runtime object records before the next map's markers are processed.
Water and mechanism runtime state also restart from the new cavern data.

Score, lives, the global frame count, and the continuing random-number stream
survive.  Marker initialization may consume that continuing RNG stream, as it
does in the original.

## Verification

Focused regression covers the no-op path, a dirty State-1-to-2 handoff with
persistent and reset fields checked separately, every automatic load through
levels 1-42, and the level-42 terminal branch.  The complete suite is then run
under strict C99 and UBSan, followed by normal and UBSan 300-tick smoke runs on
all 42 original caverns.

The principal remaining presentation boundary is the original `$15AD2`
ending animation.  Gameplay, collision, objects, items, water, HUD, camera,
level progression, and the final gameplay-to-ending control transfer are now
connected in the SDL reconstruction.
