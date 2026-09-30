# Flood Milestone 90: cycle-exact Bullfrog intro timing

## Result

The Bullfrog intro now matches the original PAL machine timing. The pre-picture
media lead is 8,354 ms, and the 1216-step visible animation advances once per
PAL video frame: 20 ms / 50 Hz. This replaces the provisional 4,000 ms lead and
10 ms / 100 Hz animation cadence.

## Measurement environment

- FS-UAE 3.2.35, A500 PAL model, cycle-exact emulation
- original Flood IPF release 504 revision 1
- Kickstart 1.3 revision 34.5 (`315093-02`)
- ROM SHA-1: `891e9a547772fe0c6c19b610baf8bc4ea7fcb785`
- audio disabled and video synchronization disabled only at the host layer;
  the emulated PAL beam, CPU, Copper, blitter, and interrupts remained timed
- deterministic replay from the same input recording and emulator state used
  for Milestone 89

The original disk image, Kickstart ROM, and executable code were not modified.

## Media lead measurement

The debugger stopped at `$B46A`, the entry that loads the 100,000-byte
`BIG_PIC`, creates its mask, installs the presentation display, and reaches the
animation. It then stopped at the first `$B870` animation-loop entry.

| Point | PAL frame | VPOS | HPOS |
| --- | ---: | ---: | ---: |
| `$B46A` entry | 1238 | `$C0` (192) | `$D2` (210) |
| first `$B870` entry | 1656 | `$5F` (95) | `$38` (56) |

Using the PAL beam position to retain the fractional frames, the elapsed time
is approximately 417.687 PAL frames:

`417.687 / 50 = 8.3537 seconds`

The SDL host therefore uses `INTRO_MUSIC_LEAD_MS = 8354`. This is the missing
media/setup interval after host-side music loading has already completed; the
earlier `$1647A` music-loader execution is not added a second time.

## Animation cadence measurement

Starting with the first hit at frame 1656, 42 subsequent consecutive `$B870`
entries were observed through frame 1698. Every interval was exactly one PAL
video frame. Beam positions stabilized near the same scan line on consecutive
hits as well, confirming a frame-synchronized loop rather than a CPU-only loop
that merely rounded to adjacent frame numbers.

At 50 PAL frames per second:

`1 / 50 = 0.020 seconds = 20 ms per animation step`

The exact recovered intro schedule contains 1216 steps, so its unskipped
visible duration is now `1216 * 20 ms = 24.32 seconds`.

## Implementation

`INTRO_FRAME_MS` in `src/main.c` is now 20, and
`INTRO_MUSIC_LEAD_MS` is now 8354. No presentation events, images, palette
data, audio replay rate, or skip behavior changed.

## Verification

- strict C99 core build and full core regression suite: pass;
- strict C99 SDL input/scheduler build and regression suite: pass;
- core regression suite under UndefinedBehaviorSanitizer: pass;
- SDL input/scheduler suite under UndefinedBehaviorSanitizer: pass.
