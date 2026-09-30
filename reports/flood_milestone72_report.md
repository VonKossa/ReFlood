# Flood Milestone 72: original gameplay sound-effect replay

## Result

Milestone 72 adds the first audio output to the SDL reconstruction. Gameplay
sound IDs now drive a software translation of Flood's original four-channel
effect tracker using the exact files and samples from the supplied disk.

## Corrected ending interpretation

The word 11 pushed at `$15B08` is passed to the disk-file loader `$E2F8`, not
to a music player. Descriptor 11 is `END_SCRE`. Therefore `$15AD2` does not
start a hidden “module 11”; it loads the ending graphics reconstructed in
Milestone 71. This corrects the provisional audio statement in that report.

## Original assets

The gameplay setup at `$164B6` loads two custom-track files:

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `FLDFX.DU` | 2,111 (`$83F`) | `5ef5e90ff1b769b0280db77b19586a20c88b81eaeadea260568ac25e28d8ac45` |
| `FLDFX.IN` | 97,348 (`$17C44`) | `a22d1db03ea27e3c1d650e671210b8ef58e45a12b6bc1a0f8ea9c9979986c1e0` |

`FLDFX.DU` contains the song header, 64 effect-track offsets, bytecode tracks,
and 13-byte volume and pitch macros. `FLDFX.IN` contains 25 sixteen-byte sample
descriptors followed by signed 8-bit sample data. Each descriptor provides the
initial and looping Paula DMA addresses and word lengths.

## Recovered player

The implementation follows `$165DC-$17530`:

- IDs 0-63 use the exact 64-entry control/next-ID table at `$16620`.
- Chained requests such as 9/10, 13/14, 60/61/62/63 start their original
  component tracks on the assigned channels.
- Track commands select delay, sample, volume macro, pitch macro, transpose,
  and note period.
- Macros preserve their signed deltas, repeat counts, durations, terminal
  state, and looping mode.
- The period table at `$16A04` is used with the PAL audio clock, 3,546,895 Hz.
- Paula channels 0/3 are mixed left and channels 1/2 right.
- Initial sample regions fall through to their original loop regions.

The SDL callback produces signed 16-bit, 44.1 kHz stereo output. It advances
the tracker at 50 Hz independently of the renderer.

## Event preservation

The previous host retained only `last_sound_id`. Milestone 72 adds a 32-entry
per-frame queue and routes every request made during that frame to the audio
engine. `last_sound_id` and `sound_pending` remain for compatibility with the
existing behavioral tests.

## Verification

Tests verify the file header, tracker speed, sound-7 channel assignment,
initial period and sample selection, the four-part sound-60 chain, simultaneous
request queuing, and a deterministic one-second PCM FNV-1a hash of `f0e93d48`.
The full suite passes strict C99 and UndefinedBehaviorSanitizer. Normal and
sanitized headless smoke runs cover all 42 maps.

The included 12-second WAV exercises sounds 7, 16, 33, 38, 49, and 60. Its
SHA-256 is `62f2d3fb628d8209f35cb4a9a59dd12da58c629c4621e8c6e994be93e0b31b94`.

## Remaining audio boundary

`TRACK.DA` and `INSTR.DA` form the separate music set loaded at `$1647E`.
Music sequencing and its transitions remain the natural scope for Milestone
73; they are not replaced with converted or guessed music in this milestone.
