# Flood Milestone 78: presentation fades and transport mosaic

## Result

Milestone 78 reconstructs two visually similar but technically independent
transition paths. General presentation changes use a Copper-palette fade at
`$A49E/$13EE4`. Paired `$9C-$AF` transports use the framebuffer transformer at
`$13FC4-$14198`; they do not alter the palette.

The SDL host now fades the selector before initial cavern setup, fades the
current cavern before each following level and before the ending, and fades the
ending after its final hold. The transport path applies its recovered mosaic
after terrain, water, objects, Matilda, Quiffy, and the transparent HUD have all
been composed.

## Copper-palette fade

`$A49E` repeats 16 passes. Each pass calls `$13EE4` for both gameplay Copper
lists and then runs the original `$1388`-iteration delay loop. `$13EE4` locates
the `COLOR00` instruction (`$0180`) and visits all 32 colour data words. For
each 12-bit `$RGB` word it independently subtracts one from every nonzero
nibble. After at most 15 decrements every palette is black; the sixteenth pass
preserves black.

The recovered call sites establish the scope:

| Caller | Effect |
| :--- | :--- |
| `$A3D2` entry | fade before initial or subsequent cavern setup |
| `$9872` path | fade before the final ending screen |
| `$9882` path | fade after the ending screen |

There is no call on the Bullfrog-intro-to-title or title-to-selector edges, so
Milestone 78 does not add speculative fades there. The SDL representation uses
one recovered colour decrement per 20 ms host presentation step.

Finishing level 42 deliberately encounters two fade loops: `$A3D2` performs
the ordinary cavern-entry fade before discovering level 43, and `$9872` then
runs the ending-branch fade while the display is already black. The host keeps
that second black delay rather than silently collapsing it.

## Paired-transport mosaic

Touching `$9C-$A5` searches for the corresponding `$A6-$AF` tile, or vice
versa, and initializes counter `$E200` to 10. In the outer frame loop
`$B230-$B28A`, that counter selects a `$13FC4` stage and then decrements.

The transformer operates on the 352-pixel, 44-byte backing rows. Because the
four bitplanes are interleaved by display line, the five cases produce these
visible results:

| Stage | Source sample | Surviving colour information |
| ---: | :--- | :--- |
| 1 | 2x1 pixels | planes 0 and 2, each duplicated into its neighbour |
| 2 | 4x1 pixels | plane 0 expanded into all four planes |
| 3 | 8x2 pixels | plane 0 expanded into all four planes |
| 4 | 16x4 pixels | plane 0 expanded into all four planes |
| 5 | 32x8 pixels | plane 0 expanded into all four planes |

Stages 2-5 are consequently black and palette colour 15; this is a deliberately
coarsening monochrome mosaic, not a dissolve and not a brightness fade.

Counter values 10 through 1 render stages `1,2,3,4,5,5,4,3,2,1`. At counter
6 the already-composed departure image receives the first stage-5 transform,
then `$B276-$B280` installs the destination coordinates. The next frame renders
the destination under the second stage 5. The reconstruction preserves that
departure snapshot while making the teleported coordinates available to game
state immediately after the frame, matching the original order.

The SDL renderer now builds the full 352-pixel backing span before cropping the
320-pixel display. This matters at stage 5: the first 16 visible pixels sample
the hidden left overfetch margin, exactly as the aligned 32-pixel group does in
the original framebuffer.

## Verification

Regression tests cover the equivalence of the existing 320-pixel renderer and
the central crop of the new 352-pixel backing, hidden-margin sampling, every
mosaic block geometry and colour mapping, fade clamping, all ten transport
stages, the counter-6 jump, and the one-frame departure snapshot. The SDL path
also passes a strict C99 syntax build independently of the headless host.
The full core suite passes strict C99 and UndefinedBehaviorSanitizer builds;
normal and sanitized headless hosts complete smoke runs on all 42 caverns.

See `presentation_transitions_milestone78.png` for exact C-rendered examples of
the normal cavern, three mosaic stages, and the palette fade.
