# Flood Milestone 71: exact `$15AD2` ending animation

## Result

Milestone 71 completes the level-42 control transfer introduced in Milestone
70. The SDL host now enters the original full-screen ending presentation,
using graphics and palette bytes extracted from the supplied disk rather than
replacement artwork.

## Recovered asset and display

The custom-track directory identifies `END_SCRE` at tracks 137-159 with an
exact size of 124,160 bytes (`$1E500`). Its SHA-256 is
`31630701739c6403f952cde6f1d8add9c75867870f15f076ee0295b584471c07`.

The first `$A500` bytes are the initial 352x240 Amiga backing screen: four
planes of `$2940` bytes, with a 44-byte row stride. The visible ending is the
leftmost 320 pixels. The remaining `$14000` bytes are a patch atlas with four
`$5000`-byte planes and a 40-byte source-row stride.

The routine first installs the black palette at `$16438`, copies the initial
screen to both display buffers, and then installs the dedicated palette at
`$16458`:

`000 310 530 752 610 930 B60 B96 231 341 461 670 310 410 510 950`

As with the now-verified gameplay graphics, plane zero supplies colour bit
zero. No bitplane permutation or palette substitution is used.

## Exact patch sequence

`$163A4` copies a rectangular patch into all four planes. Its parameters are
destination byte offset, atlas byte offset, width in 16-bit words, and height
in rows. The ending driver at `$15AD2-$163A2` makes 75 such calls.

The first 39 calls form the regular shaft-climbing loop. The remaining 36
calls are represented from their literal recovered parameters. One patch has
zero intervening VBlanks and is therefore applied in the same displayed frame
as its successor. Including the one-VBlank initial still and the final
62-VBlank hold, the presentation contains exactly 334 displayed PAL frames.

## SDL integration

After the original level-42 exit handoff sets `game_complete`, the host loads
the extracted asset and palette, changes its logical display from 320x208 to
320x240, and advances the ending at 50 Hz. Window close remains available;
gameplay input does not alter the recovered sequence.

The original routine starts music/module 11 before drawing. Audio synthesis
and module playback are outside the current SDL reconstruction, so Milestone
71 reconstructs the exact visual presentation and its timing only.

## Verification

Regression checks cover the extracted palette, the initial one-VBlank still,
event boundaries, the 334-frame completion boundary, and byte-exact FNV-1a
hashes of the planar screen at the base, climbing, clearing, and final states.
The final planar screen hash is `93ba1d50`.

The full core suite passes strict C99 with all warnings treated as errors and
again under UndefinedBehaviorSanitizer. All 42 recovered levels also pass
normal and sanitizer headless smoke runs.
