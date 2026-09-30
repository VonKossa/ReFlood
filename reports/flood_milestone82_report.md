# Flood Milestone 82: final SDL integration and release validation

## Result

Milestone 82 closes the last known fullscreen integration gap. The 334-frame
ending previously bypassed the shared host input routine and polled only for a
window-close event. It now uses the same non-text input path as the intro,
title, level banner, gameplay, `$EE` message, high-score board, and other
presentation states. F therefore toggles fullscreen during the ending as well.

The Milestone-81 controls remain unchanged: Right Ctrl is fire/confirm, Space
is not fire, and F selects borderless desktop fullscreen. Repeat keydown events
are ignored for the toggle. On the level-selector and copy-protection editors,
F remains an input character so valid text cannot be blocked.

## Aspect-ratio behavior

Every original screen size is installed with `SDL_RenderSetLogicalSize` when
that presentation begins. SDL scales the logical image to the largest fitting
desktop region and letterboxes the unused area. The source dimensions remain:

| State | Logical size |
| --- | ---: |
| Bullfrog intro | 320x96 |
| Title, level selector, high scores, and ending | 320x240 |
| Copy protection | 320x208 |
| Cavern gameplay and `$EE` panel | 320x208 |

## Deterministic SDL input regression

`tests/test_sdl_input.c` compiles the real guarded SDL host path against a
minimal test interface and validates:

- Right Ctrl sets fire;
- Space does not set fire;
- the first F press requests `SDL_WINDOW_FULLSCREEN_DESKTOP`;
- a repeated F keydown produces no additional request;
- the next fresh F press restores windowed mode;
- F on a text-entry screen emits uppercase `F` and never changes display mode.

Because the ending now calls this same tested input routine, its fullscreen
behavior no longer forms a separate untested branch.

## Verification

The normal and UndefinedBehaviorSanitizer core suites pass. The new SDL input
regression passes in both normal and sanitized builds. The SDL host passes a
strict C99 compile with warnings treated as errors, and normal plus sanitized
headless hosts complete smoke runs for all 42 caverns.

This container does not provide CMake or the SDL2 development/runtime package,
so a native linked SDL window could not be launched here. The clean release
therefore retains a real-machine SDL launch and visual/audio comparison as the
only external acceptance test, rather than claiming one was performed.
