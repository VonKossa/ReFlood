# Flood reconstruction — Milestone 99

## Single-press Escape game over

Milestones 97 and 98 mapped Escape to the death path but first set the lives
counter to one. That represented a staged final life rather than the required
immediate zero-life abort and could leave the user needing another Escape
request.

Milestone 99 makes the state transition explicit:

- the first active-gameplay Escape update sets lives to zero;
- Life Force is exhausted on the same update;
- Quiffy still enters the ordinary fall/landing and death-cross sequence;
- completing the animation keeps lives clamped at zero and ends gameplay;
- the existing zero-life path then opens credits/high scores;
- after credits, the milestone 98 PLAY LEVEL overlay still enters the selected
  cavern directly without language/copy protection.

Window close remains the application-quit operation. Escape itself does not
skip the death presentation or close the program.

## Regression coverage

- one Escape request changes a three-life game directly to zero lives;
- gameplay remains active only long enough to complete the death sequence;
- death completion leaves the lives counter at exactly zero;
- no second Escape request is supplied by the regression;
- SDL Escape mapping, post-credits overlay, and direct post-credits level entry
  remain covered by the milestone 97 and 98 tests.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
