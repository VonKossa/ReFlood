# ReFlood — Milestone 134

## Escape-death immortality blink

Milestone 134 fixes the visual state used when Escape is pressed while Quiffy
is blinking under Cocktail immortality.

Milestone 130 correctly made Escape exhaust all lives, preserve the displayed
Life Force meter, and enter the ordinary fall/cross death sequence. It also
cleared `invulnerable_timer` and `player_blink_active`, however, which made
Quiffy become continuously visible on the first Escape update.

The Escape branch now retains and freezes the active protection timer for the
death sequence. Death mode has already been entered, so the timer cannot cancel
the forced death or restore normal control. Freezing it also guarantees that a
long fall from a wall or ceiling cannot exhaust the blink before the cross
animation begins. `flood_player_should_draw()` continues to alternate
visibility with the two render buffers throughout both stages.

Unprotected Escape death remains continuously visible. Protected Escape death
still sets lives to `00`, preserves the pre-Escape Life Force meter throughout
the animation, completes the death cross, and exits gameplay normally.

## Verification

- protected Escape retains the timer and blink on the initiating update;
- the grounded handoff enters death mode 2 without cancelling protection;
- every live cross-animation update follows the alternating-buffer draw gate;
- the protection timer cannot cancel the final-life game-over;
- the existing Escape Life Force and HUD regressions remain passing;
- strict core, headless-host, and SDL-stub builds pass;
- complete core and SDL-input regression suites pass.

The package remains source-only; no executable is included.
