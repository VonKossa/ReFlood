# Flood Milestone 89: cycle-exact PAL timing calibration

## Result

Gameplay and the 51-state level banner now run at the measured original rate:
one logic update every three PAL video frames, or 60 ms / 16⅔ Hz. This replaces
the provisional 40 ms / 25 Hz host cadence.

## Measurement environment

- FS-UAE 3.2.35, A500 PAL model, cycle-exact emulation
- original Flood IPF release 504 revision 1
- Kickstart 1.3 revision 34.5 (`315093-02`)
- ROM SHA-1: `891e9a547772fe0c6c19b610baf8bc4ea7fcb785`
- audio disabled and video synchronization disabled only at the host layer;
  the emulated PAL beam, CPU, Copper, blitter, and interrupts remained timed

The debugger was stopped at the original outer gameplay-loop entry `$D7CE`.
Copy protection was bypassed by changing its transient runtime result state;
neither the disk image nor executable code was modified.

## Captured evidence

The first banner call to `$D7CE` occurred at PAL frame 5644. The next call,
which begins active gameplay after all 51 banner states, occurred at frame
5797. The difference is 153 PAL frames:

`153 / 51 = 3 PAL frames per banner update = 60 ms`

After the first active call at frame 5797, its initialization/input-overlap
iteration returned to `$D7CE` at frame 5803 (six PAL frames). The next 30
active-gameplay breakpoint frames, which form the steady-state sample, were:

`5803, 5806, 5809, 5812, 5815, 5818, 5821, 5824, 5827, 5830,`
`5833, 5836, 5839, 5842, 5845, 5848, 5851, 5854, 5857, 5860,`
`5863, 5866, 5869, 5872, 5875, 5878, 5881, 5884, 5887, 5890`

All 29 consecutive intervals in that sample are exactly three PAL frames. At
50 PAL frames per second, this is `3 / 50 = 0.060` seconds per logic update.
The six-frame transition interval is retained here because it confirms the
static audit's finding that exceptional workload can delay an unthrottled
original outer-loop iteration; it does not change the stable pacing quantum.

## Implementation

`GAME_FRAME_MS` in `src/main.c` is now 60. The shared constant already clocks
both `play_level_banner()` and the active gameplay loop, so no gameplay-state
logic or movement constants were changed.
