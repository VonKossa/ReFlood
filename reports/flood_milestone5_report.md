# Flood reverse engineering - Milestone 5

This milestone moves from rendering/input into the gameplay object layer.

## Result

The routines around `$1363A-$13DC8` operate on a common mutable object record addressed by `A0`. A high-confidence prefix layout has now been recovered, including world position at `+0/+2`, flags/state bytes at `+8/+9`, a timer at `+0A`, and sprite/type parameters including `+0E`.

A key correction to the naive model is that `+4/+6` are not universally velocity. Different object handlers reuse these slots with different semantics. The reconstructed C therefore deliberately calls them `slot04/slot06` until each handler family is understood.

The routine at `$13714` has been lifted into compiling C. It is an animation/reset state machine: on first frame it plays sound 7, advances a phase byte, draws sprites `$98-$9B`, and on phase 7 clears state and restores coordinates from `+4/+6`.

## Why this matters

We now have evidence for Flood's object architecture rather than only standalone functions. The engine appears to dispatch multiple handlers over records sharing a common header but using type-specific payload semantics.

## Next milestone

Recover `$F0E0`, the collision/geometry helper shared by most of these handlers. Its calling patterns are rich enough that understanding it should reveal the meaning of motion slots and collision flags across several object types simultaneously.
