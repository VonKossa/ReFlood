# ReFlood — Milestone 117

## Corner and apparent stuck-key correction

The host's corner-attachment helper was reusing `surface_x` and `surface_y`
captured when the attachment began. Those working values could outlive the
actual keyboard direction for as many as ten gameplay updates, making a
released key appear stuck and occasionally holding Quiffy in a corner.

The helper now refreshes both working directions from the current raw input on
every attachment update before applying the recovered contact rules. Neutral
input therefore stops cached surface motion, and a reversal takes effect on
the current update. Collision detection, the original attachment timeout, and
corner pose selection are unchanged.

## Flamethrower movement lock

When the sustained flamethrower (`selected_weapon_state == 13`) is selected and
fire is held, ReFlood now consumes horizontal and vertical player commands
before Quiffy's movement and special-effect paths run. This applies from the
first held-fire update and also to the gameplay update performed at the start
of the level banner.

The lock suppresses only player-directed movement. Gravity, water, damage, and
other non-input physics continue normally. Releasing fire restores movement on
that same update, and the other five weapon types retain their existing input
behavior.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete existing gameplay and presentation regression suites: pass;
- released attachment input replaces a cached direction with neutral: pass;
- reversed attachment input is used on the current update: pass;
- held flamethrower fire blocks left, right, up, and down: pass;
- sustained successful and malfunction flamethrower states stay fixed: pass;
- fire release restores movement immediately: pass;
- grenade movement while firing remains enabled: pass;
- level-banner gameplay update applies the same flamethrower lock: pass.
