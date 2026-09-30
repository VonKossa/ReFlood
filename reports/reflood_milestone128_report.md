# ReFlood — Milestone 128

## Level-start cue moved to the gameplay boundary

The reconstructed “Let's go” cue was started before the first frame of the
level banner. It now starts after all 51 banner frames and their final timing
dwell, immediately before control returns to the active gameplay loop. At the
normal 60 ms host gameplay step, the verified trigger point is 3,060 ms after
the banner function begins.

If the banner is interrupted because the game stops or quits, the cue is not
started.

## Original-code correction

The earlier Milestone 83 interpretation of the level-entry code was incorrect.
The live 68000 disassembly shows:

- `$AD70`: resource number 4 is passed to `$E232`;
- `$AD7C`: resource number 7 is passed to `$E232`;
- `$E232-$E2B6`: the selected resource record is loaded and installed;
- `$AD94-$B2F8`: the 51-state banner loop runs;
- `$165DC`: the separate effects playback routine.

Resource 7 is `W_BLOCKS.BLK`, so the immediate value 7 at `$AD7C` is a data
resource index, not sound effect 7. No call to `$165DC` occurs in the original
level-entry/banner range. ReFlood retains the reconstructed cue requested by
the project, but its placement is now explicitly a host presentation choice
at the banner-to-gameplay transition rather than a claimed translation of
`$AD7C`.

## Verification

- strict C99 core build with warnings treated as errors: pass;
- complete gameplay, presentation, audio, and data regression suite: pass;
- SDL host regression suite: pass;
- SDL timing regression confirms the sound trigger occurs only after all 51
  banner dwells (3,060 ms at the normal host step): pass.

The package remains source-only; no executable is included.
