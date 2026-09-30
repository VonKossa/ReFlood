# Flood reconstruction — Milestone 105

## Post-score attract sequence

After game over—or after the level-42 ending—the front end now retains the
completed score board and enters the original title/high-score attract loop.
The high-score screen is shown first. After 200 PAL presentation frames
(4.0 seconds), the display changes to the title; another 200 frames returns to
the high-score screen, and the sequence repeats.

Presentation music remains stopped throughout this post-score loop. If the
current score qualifies for the table, the name editor completes on the
high-score screen before timed alternation begins.

## Screen-dependent fire routes

Fire now records which attract screen is visible:

- on the title screen, PLAY LEVEL uses its normal clean startup background;
- on the high-score screen, PLAY LEVEL is composited over the retained score
  planes.

Both destinations are post-score routes. After the player chooses a password
or level, the selected cavern loads directly; neither language selection nor
copy protection is shown. Fire must first be released, so a held confirmation
from the name editor cannot accidentally choose a route.

The loop uses the same continuously redrawn presentation path on both screens,
so fullscreen/windowed transitions remain safe.

## Original-code basis

The front-end control loop at `$974E-$977A` repeatedly returns through the
title and high-score paths. Its input/timeout routine at `$9F62` uses the
`$186A0` polling limit before changing displays. The host represents this
duration as 200 frames on its calibrated 50 Hz presentation clock.

## Regression coverage

- the post-score loop starts on the high-score board;
- exactly 200 presentation frames select the title and another 200 return to
  high scores;
- fire during high scores selects the retained-score selector background;
- fire during the title selects the normal selector background;
- both complete integration paths require a released fire button;
- the post-ending route still enters this loop only after the ending finishes.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- headless load/run smoke test for all 42 levels: pass
