# ReFlood — Milestone 118

## Immediate jump at the top of a block

Milestone 117 correctly stopped stale tangent input, but refreshed both fields
used by the attachment path. The perpendicular field is not another live
movement direction: it retains which side Quiffy is attached to and is needed
to round the corner.

The attachment helper now refreshes only the surface tangent—X on floors and
ceilings, Y on walls—while retaining the perpendicular attachment direction.
When upward wall travel first acquires south support at the top of a block, the
wall attachment is released on that same update. The existing south-support
movement path then expands Up from `-4` to the original full `-16` jump rather
than waiting for the ten-update attachment timeout and producing small hops.

## Jump-and-fire timing

The flamethrower input lock now starts on continued held fire rather than the
initial fire edge. The first update therefore still receives Up, allowing a
correctly timed jump and fire press to initiate both actions.

Action records are not redispatched on the update where State 13 is assigned.
The flamethrower now records that Quiffy had south support on this fire-edge
update and carries that fact into the State-13 initializer on the following
update. Consequently, the jump does not incorrectly invalidate the weapon
before it can start.

## Sustained fire while falling

South support remains required to begin an ordinary flamethrower action, but
is no longer required after State 14 or the State-15 chicken malfunction has
started. If a laser bridge retracts under Quiffy, gravity pulls him down while
the active flame continues to follow his position until fire is released.
Death and fire release still terminate the action normally.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete existing gameplay and presentation regression suites: pass;
- complete climbs around both sides of a solid block: pass;
- first top-support update produces vertical velocity `-16`: pass;
- stale tangent input remains corrected without discarding attachment side: pass;
- initial Up+fire performs the jump and arms State 13: pass;
- delayed State-13 initialization retains fire-edge support: pass;
- sustained flame remains active after its supporting floor is removed: pass;
- Quiffy falls while the sustained flame follows him: pass;
- continued fire suppresses all four movement directions: pass;
- fire release and non-flamethrower movement behavior remain unchanged: pass.
