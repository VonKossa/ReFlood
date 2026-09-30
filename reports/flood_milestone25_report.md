# Flood reverse engineering — Milestone 25

## Main result: state 22 is not Sparkling Fungi

The handler at `$1363A` was re-lifted against the corrected global sprite atlas. It is a mechanical projectile/emitter hazard, not a stationary fungus.

The runtime object keeps a moving projectile coordinate in `+0/+2` and an origin/emitter coordinate in `+4/+6`. It checks a `32x8` horizontal sweep eight pixels to the right, advances `x` by eight pixels per update, and kills Quiffy instantly if its moving collision rectangle overlaps him.

The sprite base is `$14`. The corrected 16x16 atlas shows `$14-$17` as a projectile/impact/emitter group, which independently supports the code interpretation.

After terrain/player impact, byte `+8` is set and the next update jumps into the already recovered reset/explosion routine, which eventually restores the saved origin.

Therefore the old provisional `state 22 = Sparkling Fungi` mapping should be deleted. Current conservative name: **horizontal bolt launcher trap**.

## Three-hit object

The `23 -> 24 -> 25 -> 26` weapon-target chain is still not named. A fresh scan confirms there is no direct literal state-23 assignment in the main executable. Its creation is indirect/computed or occurs through a secondary reveal/spawn path.

At this point dynamic tracing is likely higher leverage than additional blind static scans: watch writes to `runtime_object[i].behavior` and break on value 23.

## Artifacts

- `src/bolt_trap_1363a.c`: portable semantic C lift.
- `include/flood_bolt_trap.h`: typed record/hook interface.
- `state22_reclassification.md`: evidence and correction.
- `state23_search_status.md`: current narrowing result.
- `evidence/state22_sprites_14_17.png`: corrected sprite group.

## Next target

1. Find the actual Sparkling Fungi handler by searching stationary instant-kill objects with the corrected atlas.
2. Use runtime write tracing (WinUAE or equivalent) to catch the first creation of behavior state 23.
