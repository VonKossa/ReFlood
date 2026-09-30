# Flood Milestone 80: final instruction-level gameplay/render timing audit

## Result

Milestone 80 traces the complete active-frame path from `$AD9E` through
`$B2FC` and corrects every demonstrated ordering mismatch in the SDL/core
model. The reconstruction now has an explicit 50 Hz clock, the recovered
51-frame level banner, post-movement item/object collision timing, an immutable
per-frame terrain snapshot, original sprite/water/HUD layer order, a true
mid-dispatch `$EE` pause/resume, and one forward object-record dispatch pass.

## Active-frame instruction order

| Order | Original path | Reconstructed operation |
| ---: | :--- | :--- |
| 1 | `$AD9E/$D7CE` | Sample input; update `$DC` auxiliary state and special item path; update Quiffy |
| 2 | `$D94A-$D990` | Derive and clamp camera from Quiffy's new coordinates |
| 3 | `$ADB2-$AEBC` | Snapshot camera/buffer phases and compose 22x14 terrain overfetch |
| 4 | `$AECE/$CAD2` | Ordinary item, eight action records, then 128 object records |
| 5 | `$CDF0-$D186` | Advance death presentation; Matilda; Quiffy/effect drawing |
| 6 | `$AEF6-$AF2A` | OR dynamic overlay/water into destination planes 2 and 3 |
| 7 | `$AF3C-$AF70` | Advance water state and speed/pause counters |
| 8 | `$AF76` | Draw the transparent HUD at visible Y=0 |
| 9 | `$B206-$B28A` | Handle level sentinel and transport mosaic/teleport |
| 10 | `$B290-$B2DA` | Present the completed buffer, toggle buffer index, advance tile phase |

The old host ran the ordinary item/action/object dispatch before Quiffy. That
made cell entry and enemy collision one frame late or early depending on the
effect. It now follows `$D7CE` first. A pickup in the cell Quiffy enters is
therefore handled on that frame. Conversely, an object that reduces Life
Force below zero does so after the player update, and death mode begins on the
next active frame.

This order also exposes two smaller exact effects. A newly collected Cocktail
sets its timer to 50 after the Cocktail-restoration pass, so gauge restoration
begins on the following frame. When death-mode restoration overlaps Matilda's
delayed sample, restoration happens first and Matilda can immediately remove
10 Life Force afterward.

## Terrain and layer snapshots

`$EC4E` finishes the base terrain blit before `$CAD2` runs. Ordinary pickups,
triggers, actions, and mechanisms may mutate the map during `$CAD2`, but those
changes cannot alter the already-built frame. `FloodWorld.render_terrain`
now preserves this immutable terrain state. Such mutations appear on the next
frame. The independent `$FC6C` path is earlier inside `$D7CE`; consequently a
consumed `$DC` tile is absent from the current terrain snapshot.

The prior SDL compositor applied water before actors. The instruction order is
the reverse: actions, objects, Matilda, and Quiffy draw during `$CAD2`, then
`$ED66` ORs the dynamic overlay into planes 2 and 3. Water can therefore alter
actor pixels. The HUD is later still and remains transparent at the utmost top.
The corrected layer order is:

`terrain -> actions -> objects -> Matilda -> Quiffy/effect -> water -> HUD -> transport mosaic`

## Exact object-record pass

The original `$CC56-$CDA0` dispatcher walks records 0 through 127 once and
uses each record's state when that slot is reached. The core previously ran
state-family sweeps. Those sweeps had local safeguards for known transitions,
but they were not instruction-equivalent for all cross-record combinations.
The active path now performs one forward slot pass. A state written into a
later record can execute when that later slot is reached; a state written into
an already visited record waits until the next frame. State 24 still changes
only to State 12 during its visit.

## `$EE` is a resumable frame

The `$EE` call to `$99AC` occurs inside `$FDA0`, before action and object
dispatch. Milestone 79 correctly delayed the score but allowed the remainder
of the core tick to finish before displaying the modal. Milestone 80 now stops
at the actual call boundary. While the panel is active, the frame counter,
actions, objects, Matilda, water, transport, and render phases do not advance.

After fire acknowledgement, the host adds 20 points at `$FF00`, resumes with
the action and object passes, completes water/HUD/transport work, presents the
restored cavern, and only then begins another Quiffy update. Sounds requested
by the resumed half are forwarded without replaying sound `$36`.

## Recovered 51-frame level banner

The local counter initialized at `$AD94` is rendered for values 50 down through
0, giving 51 displayed frames—not 50. The moving group begins at X=320 and
advances four pixels left per frame, reaching X=120 on the last banner frame.
It draws the `$4A/$4B` strip, the `$47,$48,$49,$48,$47` decoration, and two
`$3D+digit` level glyphs.

Only the first banner frame calls `$D7CE`; the following 50 frames hold Quiffy,
items, actions, objects, Matilda, and water. All 51 frames still present and
advance the double-buffer and animated-tile phases. The same sequence now runs
after copy protection and after every successful level handoff.

## 50 Hz scheduling

The original timing unit is one PAL VBlank. Gameplay had relied on
`SDL_RENDERER_PRESENTVSYNC`, which makes speed depend on a 60 Hz, 75 Hz, or
other host display. The renderer no longer requests vsync and every gameplay,
banner, and presentation step is scheduled against an explicit 20 ms clock.
Long modal or fade paths reset the gameplay deadline instead of attempting a
rapid catch-up burst.

## Verification

Focused regressions cover movement-before-item entry, current/next-frame
terrain visibility, immediate `$DC` removal, delayed Cocktail restoration,
object-damage-to-death latency, death-restoration/Matilda order, the complete
51-frame banner hold, and true `$EE` pause/resume with a stopped object and
single frame-counter advance.

The full core suite passes strict C99 and UndefinedBehaviorSanitizer builds.
Normal and sanitized headless hosts complete smoke runs on all 42 caverns, and
the SDL path passes a strict C99 syntax build. The preview
`gameplay_render_timing_milestone80.png` is generated through the corrected C
core and shows banner frames 1, 26, and 51 followed by the first active frame.
