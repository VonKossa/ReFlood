# Flood reconstruction — Milestone 104

## Level-42 and ending control flow

Completing level 42 again enters the complete 334-frame ending animation. The
failure was caused by a framebuffer regression introduced alongside the
milestone-103 viewport correction: the title height changed from 240 to 208,
but the shared presentation framebuffer was still allocated as
`320 * FLOOD_TITLE_H`. The preceding Bullfrog intro retains a 320x240 canvas,
so it wrote 32 scanlines past that allocation and could corrupt later host
state.

The shared framebuffer is now explicitly sized for the largest presentation
stage, 320x240, and is no longer shrunk for the 320x208 ending. The level-42
handoff therefore reaches the ending without damaged state.

The post-ending destination is also corrected. After the ending animation and
its fade complete, the host opens the same credits/high-score screen used after
the last life. Dismissing that screen preserves its planes behind the PLAY
LEVEL panel and loads the chosen level directly, matching the established
post-score route. Only an explicit window-close request exits.

## Fullscreen title restoration

Changing between desktop fullscreen and windowed mode intentionally clears
both SDL renderer buffers. Unlike the interactive selector and high-score
screens, the static title had only been rendered once before its input loop.
After a mode switch it therefore remained black.

The title is now presented on every wait-loop iteration. A fullscreen or
windowed transition can clear stale buffers, and the current title frame is
immediately restored.

## Regression coverage

- the presentation framebuffer is fixed at 320x240 and is larger than every
  320x208 front-end viewport;
- the full 320x240 intro is rendered through the shared-buffer test path;
- a completed, non-quit ending is required to select the high-score route;
- incomplete or explicitly quit endings cannot enter that route;
- the static title presentation is checked after the fullscreen state tests;
- the existing exact ending, title, selector, and high-score tests remain
  unchanged.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- headless load/run smoke test for all 42 levels: pass
