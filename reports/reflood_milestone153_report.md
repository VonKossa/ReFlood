# ReFlood Milestone 153 Report

## Conveyor movement rebuilt from the original

Milestone 153 discards the conveyor changes from milestones 151 and 152 and
traces the complete original path on the real level 1 conveyor geometry:

- diagonal collision probes at `$F22C`;
- conveyor corrections at `$F4B2/$F4DE`;
- contact/cache handling at `$DAEC-$DBE6`;
- final horizontal movement at `$DF02`.

The original runtime trace in the right-moving level 1 passage produced:

- contact mask `$BB` with six contacts;
- corrected horizontal input `+2` with no key held;
- final horizontal displacement `+8` pixels.

The two-unit bias is intentional. The one-cell passage has a conveyor surface
on both the ceiling and floor, and each contributes one unit. Therefore:

| Input | Corrected input | Movement |
| --- | ---: | ---: |
| Against the belt | +1 | +4 pixels |
| None | +2 | +8 pixels |
| With the belt | +3 | +12 pixels |

The mirrored passage produces `-4/-8/-12` pixels.

## Cause

The generic tight-corridor handoff added for crawling aligned Quiffy correctly,
but then replaced the collision-adjusted horizontal direction with the raw
keyboard/gamepad direction. In a conveyor corridor this erased both belt
corrections.

That explains every reported milestone-150 symptom:

- opposite input allowed Quiffy to walk against the belt;
- matching input used ordinary walking speed and appeared slower than neutral;
- neutral input still retained the conveyor speed because the handoff was not
  entered without a horizontal command.

Milestone 151 changed animation phase even though animation was not the cause.
Milestone 152 additionally reversed the original correction signs. Both changes
are removed in milestone 153.

## Correction

The corridor handoff now preserves the live direction emitted by the original
collision/contact calculation before cached corner directions are replayed.
Ordinary tight corridors are unchanged because their corrected direction equals
the requested input. Conveyor passages retain their complete ceiling-plus-floor
bias.

The conveyor sound remains enabled as the previously agreed intentional fix to
the silent Amiga behavior. No level-specific gameplay exception was added.

## Verification

- Real extracted level 1 map geometry is tested in both conveyor directions.
- Right-moving passage: opposite/neutral/matching movement is `+4/+8/+12`.
- Left-moving passage: matching/neutral/opposite movement is `-12/-8/-4`.
- Exact original contact mask `$BB` and six-contact geometry are asserted.
- Strict C99 core suite: pass.
- SDL input/settings suite: pass.
- UndefinedBehaviorSanitizer core suite: pass.
- Strict headless build and all 42 level smoke runs: pass.

The package remains source-only; no executable or build directory is included.
