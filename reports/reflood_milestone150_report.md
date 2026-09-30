# ReFlood Milestone 150 Report

## Moving crawl pose

Milestone 150 fixes the animation used when Down and Left or Right are held
together inside a one-tile-high corridor. Squiffy now moves horizontally while
remaining in the crawl animation.

## Cause

The corridor handoff introduced for passage entry correctly clears its
vertical collision component. This prevents a simultaneous Down command from
forming a diagonal movement vector that collides with the corridor floor.
However, ReFlood then passed that cleared movement value to the pose selector.
The renderer therefore saw no Down request and selected the ordinary standing
walk frames.

The original pose routine at `$D4F2-$D60C` tests the live joystick Y word at
`$17E7A` independently of the later collision result. On south support, a
positive value selects the `$4C` crawl family. Collision suppression must not
erase that animation input.

## Correction

The post-attachment vertical request is preserved for pose selection before
the passage handoff modifies the collision direction. The handoff can still
set vertical motion to zero, while Down continues to select the crawl frame
family. Horizontal movement, collision geometry, and animation advance remain
unchanged.

The correction is general and contains no level-specific conditions.

## Verification

- Level 8 Down-only input selects crawl sprite `$9C`.
- Down+Right moves four pixels horizontally, remains vertically aligned, and
  selects the right-facing crawl family beginning at sprite `$A0`.
- Down+Left moves four pixels horizontally, remains vertically aligned, and
  selects the left-facing crawl family beginning at sprite `$9C`.
- All previous tight-passage, corner, wall-top, and flamethrower regressions
  remain enabled.
- Strict C99 core suite: pass.
- SDL input/settings suite: pass.
- UndefinedBehaviorSanitizer core suite: pass.
- Strict headless build and all-level load/smoke run: pass.

The package remains source-only; no executable is included.
