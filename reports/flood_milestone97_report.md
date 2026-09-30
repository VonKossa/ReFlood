# Flood reconstruction — Milestone 97

## Scope

Milestone 97 corrects two separate front-end behaviors:

1. the destination and retained background after the credits/high-score page;
2. the gameplay behavior of the Escape key.

## PLAY LEVEL after credits

Milestone 94 redirected the completed high-score path through the title screen.
That transition was incorrect. The post-score reset still restores three lives,
score zero, and cavern 1, but control now enters the PLAY LEVEL selector
directly.

The original screens share `TEMPFILE.SCR` as their planar work buffer. The
score routine draws the credits/high-score content into that buffer, after
which the level routine overwrites only its six-row panel. Milestone 97 models
that persistence explicitly: it copies the finished score planes into the
level-selector state and redraws the panel over them. No title frame or clean
`TEMPFILE.SCR` background is inserted between the two screens.

## Escape ends the game

Escape was previously unmapped; the host-only R key merely requested an
ordinary single-life death. Escape now requests the original user-visible
abort behavior through the gameplay death machinery:

- the current attempt is marked as the final life;
- Life Force becomes exhausted;
- Quiffy follows the normal falling/grounding and death-cross animation;
- completion of that death reduces lives to zero and opens the
  credits/high-score screen.

This is not an application quit. SDL window close remains the explicit quit
event, and Escape is ignored as an abort command by non-gameplay modal screens.

## Regression coverage

- SDL Escape state maps to the death/abort input without setting quit;
- the former R test binding no longer requests a restart;
- an Escape request with three lives forces the final-life path;
- the death cross completes before `running` becomes false with zero lives;
- a post-score selector retains score-screen pixels outside its panel;
- the PLAY LEVEL panel is redrawn unchanged over the retained score planes;
- the existing score reset still restores cavern 1, score 0, and three lives.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
