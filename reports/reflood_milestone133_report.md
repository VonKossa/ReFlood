# ReFlood — Milestone 133

## Complete original fade-out routing

Milestone 133 restores the two presentation fades omitted from the SDL host.
Both use the existing 16-step fade-to-black implementation reconstructed in
milestone 78.

### Final death to credits/high scores

The original active-play loop exits through `$B2FC`, which calls the fade
driver at `$A49E` whenever its transition state is nonzero. This includes the
final-life death route. ReFlood now fades the last rendered cavern after the
fall/cross death lifecycle has completed and before stopping gameplay audio or
constructing the credits/high-score screen.

The fade is skipped when the user closes the window and remains distinct from
the existing level-42 ending path.

### Copy protection to gameplay

The original initial front-end route calls `$A49E` at `$AD66` after the
language/copy-protection sequence. ReFlood now fades a successfully completed
copy-protection display before replacing its texture with the gameplay view
and showing the level banner.

A failed challenge or window close does not perform this gameplay-handoff
fade. The direct post-score level-selection route continues to bypass copy
protection altogether.

## Corrected original call-site map

The live original has five direct callers of `$A49E`:

- `$A3D6`: level selection/front-end to cavern preparation;
- `$AD66`: successful copy-protection exit;
- `$B2FC`: common gameplay exit, including final death and level completion;
- `$9872`: pre-ending transition;
- `$9882`: post-ending transition.

No active gradual fade-in counterpart was found. New displays install fresh
Copper palettes after the fade to black.

## Verification

- transition predicates cover successful, failed, and quit paths;
- the fade regression confirms 16 presented steps over 320 milliseconds;
- coloured input pixels finish at opaque black;
- strict core and SDL-host builds pass;
- complete core and SDL-input regression suites pass.

The package remains source-only; no executable is included.
