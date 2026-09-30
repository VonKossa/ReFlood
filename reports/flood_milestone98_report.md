# Flood reconstruction — Milestone 98

## Direct level entry after credits

Milestone 97 restored the credits/high-score background and drew the PLAY LEVEL
panel over it, but the shared selector exit still entered the startup-only
language and copy-protection screen.

Milestone 98 separates the two selector routes:

| Selector entry | Background | Destination after selection |
| --- | --- | --- |
| Initial startup | Title/front-end work screen | Language and copy protection, then the selected level |
| After credits/high scores | Retained credits/high-score screen | Selected level directly |

The post-score path sets a one-use direct-level state immediately before it
returns to PLAY LEVEL. After the selected cavern is loaded, that state bypasses
the complete protection block and is cleared before the level banner and
gameplay begin. The ordinary startup state remains false and therefore keeps
the existing language/copy-protection sequence.

This change does not alter the separate milestone 97 Escape behavior: Escape
still completes Quiffy's final-life death sequence, opens credits/high scores,
and then reaches the overlaid PLAY LEVEL selector.

## Regression coverage

- initial-startup selector route requires language/copy protection;
- post-credits selector route bypasses language/copy protection;
- credits/high-score background retention and PLAY LEVEL panel overlay remain
  covered by the milestone 97 pixel regression;
- Escape remains mapped to the final-life death path rather than application
  quit.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
