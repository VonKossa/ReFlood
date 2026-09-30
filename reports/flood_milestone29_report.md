# Flood reverse engineering — Milestone 29

## Main result: runtime state 19 is the Mine

The runtime dispatcher sends state 19 to `$FD28`. Marker 19 is present 109 times across the decoded level set and its initializer creates runtime state 19 while replacing the level marker with tile `$E6`.

`$FD28` tests a tiny player overlap rectangle at `(x+4,y+12)` with dimensions `8x4`. On contact it clears the corresponding map cell, advances the object from state 19 to state 20, resets its animation byte, and shifts the object origin by `(-8,-16)` so the next state is centered over the contact point.

State 20 dispatches to `$1091A`, the already-recovered four-phase explosion routine. On its first explosion phase, a 32x32 overlap removes 40 Life Force. The routine then advances into state 21, the bouncing Heart pickup.

This is a direct semantic match for the manual's Mine: a stationary small object that goes "Boom!" and removes Life Force.

## Unexpected but binary-verified behavior

The mine reuses the same state-20 explosion path used by Heart-producing enemy deaths. Consequently a detonated mine eventually becomes a Heart. This is odd from a modern design perspective, but it is what the state machine does.

## State 11 is no longer a serious Sparkling Fungi candidate

State 11 calls `$1306A`, uses subtype `$4E`, and belongs visually/structurally to the `$CB` mechanical family. More importantly, none of the 42 decoded levels contains marker 11 and no decoded trigger creates marker 11. It may be a dormant/legacy mechanism or a state entered indirectly, but there is no evidence that it represents a rare cast member placed in the shipped levels.

## Sparkling Fungi search is narrower

The manual says Sparkling Fungi are stationary and instantly drain all Life Force. With state 19 identified as Mine and state 11 absent from shipped level placement, the strongest remaining possibilities are:

1. an ordinary terrain tile with a special contact path outside the normal runtime-object table;
2. a secondary/static-object subsystem rather than the 128-record creature dispatcher;
3. a rare special marker that is transformed before the marker scan sees it.

The decoded tile atlas and 256-byte attribute tables now make option 1 testable systematically.

## Artifacts

- `src/mine_state19.c` — semantic C lift of `$FD28`.
- `include/flood_mine.h` — recovered data/API definitions.
- `mine_state_flow.md` — state 19 -> 20 -> 21 lifecycle.
- `state11_status.md` — why state 11 is being removed from the Fungi shortlist.
- `evidence/mine_tile_e6_ABC.png` — tile `$E6` from all three block banks.
