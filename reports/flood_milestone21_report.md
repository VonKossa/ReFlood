# Flood reverse engineering — Milestone 21

## Summary

This milestone closes the common weapon-hit victim transitions. The key result is that Flood has **two shared post-hit death paths**, plus one unusual multi-hit static object chain.

## Runtime dispatcher

The object update loop at `$CC50` reads behavior state `+0x0C`, then indexes a PC-relative state table. Cross-checking this table against `$F9E6`'s weapon-hittable states gives an exact transition map.

### Heart-producing deaths

Hittable states **1** and **12** become states **2** and **13** on a weapon hit. Both post-hit states dispatch to `$1091A`.

`$1091A`:

- plays sound 8 when its animation begins;
- runs a four-step death animation;
- on phase 4 sets behavior state to **21**;
- nudges X by +4;
- gives the resulting object an initial upward motion value of `-12`;
- resets its timer and seeds animation state `40`.

State **21** dispatches to `$FB42`, whose behavior confirms it is a **Heart pickup**. On player overlap it:

- awards **+10 score**;
- restores **+64 Life Force**;
- clamps Life Force to **511**;
- clears the object.

Thus two enemy families have a weapon-death path that literally transforms the dead enemy slot into a bouncing Heart pickup.

### Non-Heart deaths

Hittable states **5**, **8**, and **14** become states **6**, **9**, and **15**. All three dispatch to `$10A10`.

`$10A10`:

- plays sound 7 on entry;
- advances through seven animation phases;
- draws sprites derived from `$98 + (phase >> 1)`;
- retains a damaging overlap test during the sequence;
- after phase 7 clears behavior/timer/animation and removes the object.

This is therefore a common hazardous death/explosion path rather than a Heart drop.

## The 23 -> 24 -> 25 -> 26 object

The special chain is now structurally clear even though its historical object identity remains unresolved.

States **23**, **24**, and **25** are all weapon-hittable. However:

- state 23 has no ordinary behavior handler;
- state 24 has no ordinary behavior handler;
- states >=25 bypass the normal 0..24 dispatcher table;
- a weapon hit is what advances 23 -> 24 -> 25 -> 26;
- state 26 is no longer accepted by `$F9E6`.

That means this is best described for now as a **three-hit static destructible object**. It does not behave like a normal mobile enemy whose post-hit state animates itself. The final state 26 appears inert/non-targetable from this dispatcher path.

Calling it a boss would be premature; there is no evidence here of movement or an autonomous boss state machine.

## Combat loop now recovered

The common combat path can now be written conceptually as:

```c
victim = find_weapon_target(hitbox);
if (victim) {
    victim->behavior++;
    victim->animation = 0;
    score += weapon_score;
}

/* object dispatcher */
switch (victim->behavior) {
case 2:
case 13:
    death_to_heart(victim);
    break;
case 6:
case 9:
case 15:
    death_explosion(victim);
    break;
}
```

For Heart-producing enemies the path continues:

```text
weapon overlap
  -> behavior state + 1
  -> death animation $1091A
  -> state 21 Heart
  -> bouncing Heart $FB42
  -> player pickup
  -> +10 score, +64 Life Force (max 511)
```

This is the first nearly complete weapon-to-reward combat lifecycle recovered from the binary.

## Files

- `src/enemy_death_states.c` — portable semantic lifts of `$1091A`, `$10A10`, and the confirmed Heart pickup effect
- `include/flood_enemy_death.h` — reconstructed 20-byte runtime object prefix and combat globals
- `post_hit_dispatch_map.md` — exact hittable-state -> dispatcher map
- `RUN_PROG.bin` — original extracted program used for this analysis

## Next target

The highest-value next pass is to identify **which normal dispatcher states correspond to which named creatures**, then connect the Heart-producing vs non-Heart-producing death paths back to Beady Ball, Psycho Teddy, and the remaining creature handlers. In parallel, tracing creation of state 23 should reveal the three-hit destructible object's identity.
