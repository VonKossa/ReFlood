# Flood Milestone 69: exact `$DC` auxiliary States 1 and 2

## Result

Milestone 69 closes the auxiliary 18-byte record at `$7DA46`.  The State-0
`$DC` activation added in Milestone 68 now continues through the complete
State-1 routine at `$1019A-$10380` and State-2 routine at `$1046E-$104F8`.

## State 1: controllable bounce form

State 1 rebuilds Quiffy's contact ring and retains only south support and the
fatal-terrain bit (`contacts & $0110`).  While it remains active it selects
pose `$5E + horizontal bank + animation phase`; the special `$50` renderer
therefore draws original global sprites `$AE-$B5`.

On south support, UP subtracts four from the stored velocity down to -24,
copies it into both the auxiliary record and Quiffy's vertical velocity, and
clears the contact word.  Releasing UP adds four toward zero.  A nonzero
support velocity requests sound `$21`.  A Quiffy downward velocity of at least
14 primes -20, while the secondary velocity word below -16 is fed one unit
toward -16 per visit and clamps Quiffy's current vertical velocity to -16.

FIRE sets the shared fire counter to 2.  If the map cell at Quiffy `(+16,+24)`
is empty, State 1 ends and `$DC` is restored there; a blocked cell leaves the
effect active.  Death mode 1 also cancels the record without restoration.

Fatal-terrain contact is consumed by this subsystem rather than entering
Quiffy's ordinary death path.  It saves Quiffy's coordinates, clears the
trajectory phase, installs the 58-visit lifetime, and changes to State 2.  As
with the original dispatcher, State 2 does not execute until the next visit.

## State 2: fixed trajectory

The signed-word table at `$10382-$10469` contains 58 X/Y pairs.  Each State-2
visit adds one pair to the auxiliary position, advances the byte offset by
four, draws global sprite `$B4`, decrements the lifetime, and replaces
Quiffy's normal update.  It also sets the same dispatcher-suppression flag used
by the Orange Can, so ordinary items, weapons, actions, enemies, mechanisms,
and their rendering are skipped on that frame.

All 58 recovered pairs are embedded without interpolation.  Their net motion
is `(+32,-96)`.  The last visit draws `$B4` and only then clears State 2; the
SDL host preserves this with a separate render snapshot.

## Verification

Focused regressions cover State-1 acceleration/deceleration, sound timing,
sprite selection, empty-cell restoration, death cancellation, fatal-bit
consumption, the delayed 1->2 transition, every one of the 58 trajectory
coordinates, phase and lifetime values, dispatcher suppression, and the final
draw before clear.  Strict C99 and UBSan tests pass, followed by normal and
UBSan smoke runs over all 42 original levels.

The next gameplay milestone can now implement the `$88` completion handoff:
automatic level increment, state reset, loading the next of the 42 recovered
maps, and the final-level outcome.
