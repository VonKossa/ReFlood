# Flood reverse engineering - Milestone 20

## Scope

This pass decoded `$F9E6`, the common target query called by grenades, boomerangs, shuriken, dynamite, the flamethrower and the radial weapon/effect.

## `$F9E6` is the combat victim query

The routine scans **128 runtime object slots** starting at `$7C6F6`. Each object record is **20 bytes**. It uses the object's sprite ID at `+0x0E` to fetch the 8-byte descriptor at `$7D186`; `row_bytes * 8` provides the object's current pixel width and descriptor word `+2` supplies its height.

The four call arguments are:

```text
F9E6(x, y, width, height)
```

The function returns the **first matching object index** in `D0`, or `-1` if no hittable object overlaps the rectangle.

## Hittable behavior states

The object field at `+0x0C` is now definitively a **behavior/state selector**, not a generic hit-point counter. `$F9E6` only accepts these state values:

```text
1, 5, 8, 12, 14, 23, 24, 25
```

All other states are skipped by a compact state-filter jump table.

This is important because it shows that combat is state-machine driven. The routine does not ask "does this enemy have HP left?"; it asks "is this object currently in a weapon-hittable behavior state?"

## Exact overlap model

For X, the routine computes:

```text
dx = query_x - object_x
```

If `dx < 0`, overlap is accepted when `dx >= -query_width`. If `dx >= 0`, overlap is accepted when `dx <= object_sprite_width`.

Y is tested identically using `query_height` and the object's sprite height. This is an inclusive AABB-style overlap test using the query rectangle on the negative side and current sprite dimensions on the positive side.

## What a weapon hit actually does

Every decoded weapon caller follows the same pattern after `$F9E6` returns a non-negative index:

```c
object = &objects[index];
score += amount;
object->behavior_state += 1;
object->animation09 = 0;
```

Most attacks award `+10` score for the hit. The large dynamite explosion path awards `+20`.

Thus there is **no universal `enemy->hp -= damage` operation** here. A successful weapon strike advances the victim to its next behavior state and resets its animation phase. For the common enemy classes, the next state is a hit/death/transition state handled by the object dispatcher.

## Runtime object dispatcher cross-check

The global object update loop also reads `+0x0C` as its dispatch state and uses it to select behavior handlers. That independently confirms the semantic interpretation above.

This corrects earlier provisional language that treated `+0x0C` as a damage counter in some contexts.

## Durable/special states

States `23`, `24`, and `25` are all independently accepted by `$F9E6`. That is unusual: a hit can advance 23 -> 24 -> 25 -> 26 while the intermediate values remain targetable. This strongly suggests a multi-stage/durable special object or encounter, but its identity is intentionally left unresolved until those states' creation/update paths are traced.

## Combat architecture recovered so far

```text
weapon state machine
       |
       v
projectile/explosion hit rectangle
       |
       v
$F9E6 scans 128 object slots
       |
       v
hittable-state filter
       |
       v
sprite-sized AABB overlap
       |
       v
return victim index
       |
       v
score += 10 (20 for large dynamite blast)
behavior_state++
animation = 0
       |
       v
object dispatcher runs victim transition/death state
```

## Next target

The next high-value pass is to trace the post-hit dispatcher states for the five common hittable classes (`1,5,8,12,14`) plus the special `23/24/25` sequence. That should identify which transitions are enemy deaths, which create score/Heart effects, and whether the 23-25 chain is a boss or destructible multi-hit object.
