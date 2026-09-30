# Flood reverse engineering - Milestone 19

## Scope

This pass traced the selected-weapon word `$E1EE` into the player firing dispatcher at `$CB40` and followed all six exchangeable weapon codes into their state-machine handlers.

## Fire dispatcher

The action-state word at offset `+0x0A` drives an 18-entry jump table. State zero copies the selected weapon code from `$E1EE`; subsequent states belong to that weapon until the handler resets `+0x0A` to zero.

This explains the unusual selected weapon codes `1,4,8,10,13,16`: they are not arbitrary item numbers. They are **entry state numbers** into one shared firing-state jump table. Gaps between entry numbers are occupied by that weapon's continuation/explosion states.

## Named weapons recovered

Five state families map cleanly to the five weapons documented in the Flood manual:

- code 1 / tile `DD`: Grenade
- code 4 / tile `DE`: Boomerang
- code 8 / tile `EB`: Shuriken
- code 10 / tile `EC`: delayed-action Dynamite
- code 13 / tile `ED`: Huge Flamethrower

The mappings are based on movement, collision, lifetime, and rendering behavior rather than icon appearance.

## Sixth exchangeable state family

Code 16 / tile `EA` remains intentionally unnamed. It creates a single latched radial effect that expands in eight directions for about ten steps. This is clearly a weapon/action and clearly exchangeable, but the standard manual only names five pieces of dangerous hardware. We should preserve this discrepancy and investigate the sprite/map art or an alternate manual before assigning a historical name.

## Engine insight

The selected weapon code doubles as the firing state-machine entry point. This is a compact 68000-era design: each weapon occupies a contiguous slice of one dispatcher, so no separate weapon vtable is needed.

## Next target

The best next pass is the combat victim side: `$F9E6` and the object-table damage paths touched by projectiles. That should recover the enemy hit/damage model, weapon damage values, death transitions, and Heart drops.
