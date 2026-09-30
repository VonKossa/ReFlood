# ReFlood — Milestone 130

## Escape-death Life Force meter preserved

The Escape abort previously forced `player.life_force` to `-1` in order to
enter the death path. That made the Life Force meter empty immediately, even
though the original leaves the meter at its current value during the complete
fall and cross animation.

Escape now performs the three independent operations explicitly:

1. The lives counter is set to `00` immediately.
2. The current Life Force value is captured and retained throughout death
   modes 1 and 2.
3. The ordinary death-completion path restores Life Force to full immediately
   before the screen transitions away from gameplay.

Cocktail protection is still cleared, so immortality cannot cancel an Escape
abort. Damage, drowning, or actor updates that occur while the body is falling
cannot change the captured HUD value.

## Verification

- strict C99 core build with warnings treated as errors: pass;
- complete core regression suite: pass;
- SDL host regression suite: pass;
- Escape at partial Life Force sets the HUD lives digits to `00` while every
  byte of the rendered Life Force meter remains unchanged: pass;
- the meter remains unchanged on every fall and cross-animation update: pass;
- the final death update changes the meter to full and ends the game: pass;
- the same behavior with active Cocktail immortality: pass.

The package remains source-only; no executable is included.
