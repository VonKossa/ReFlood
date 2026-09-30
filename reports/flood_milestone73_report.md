# Flood Milestone 73: original presentation-music replay

## Result

Milestone 73 reconstructs Flood's separate `TRACK.DA` / `INSTR.DA` music
path. The supplied disk data is decoded directly and replayed by the existing
four-channel software translation of the original driver. No replacement
module, samples, or guessed command semantics are used.

This is presentation/title music, not background music for the caverns. The
original setup at `$16478` loads this pair, initializes the driver at `$16AEE`,
selects song 0 at `$16E56`, and starts all four channels through `$16F16` with
control `$A0`. The gameplay entry at `$981C` instead calls `$164B6`, which
loads the `FLDFX` pair reconstructed in Milestone 72. `$164E4` shuts the active
audio path down at the recovered transitions. The reconstruction preserves
that separation.

## Exact assets

`TRACK.DA` is packed on disk. Its first two longwords give packed size `$1733`
(5,939 bytes) and unpacked size `$2480` (9,344 bytes). Its stream alternates
literal runs and copies from earlier output, the same back-reference scheme
used by the level data. `INSTR.DA` begins with `$1D6E0`; including that longword,
the driver-visible region is `$1D6E4` bytes. The remaining disk-file tail is
padding and is not fed to the player.

| Extracted file | Bytes | SHA-256 |
| --- | ---: | --- |
| `TRACK_DA.bin` | 9,344 (`$2480`) | `485a6dd89a869258e296fbacab57ab50c2211c718f494c70c758987aff6b2c41` |
| `INSTR_DA.bin` | 120,548 (`$1D6E4`) | `64242ab19521c0247eae8cfff2ef623099827e7573a9e6d280e856aeb140a5f7` |

The extraction script validates both decoded lengths before writing either
music asset.

## Song structure

Song 0 starts at file offset 2. Its header contains speed 3, volume-macro table
`$20F5`, pitch-macro table `$2295`, and four sequence pointers:

| Paula channel | Sequence start | Entries before `$FF` |
| ---: | ---: | ---: |
| 0 | `$0138` | 44 |
| 1 | `$0165` | 44 |
| 2 | `$0192` | 49 |
| 3 | `$01C4` | 42 |

The union of the sequences references 121 tracks, spanning IDs `$01-$92`.
Each sequence entry indexes the word-offset table at `$0012`; each selected
track runs until its own `$FF`. At that marker, `$17284` advances the channel's
sequence pointer. The sequence-ending `$FF` returns to its saved start because
control `$A0` supplies the loop bit.

All reachable tracks were decoded structurally. They contain 2,320 note
commands and reference instruments 0-17. Their only extended opcodes are 121
transpose commands (`$E0`) and 143 pitch-macro commands (`$E1`). Therefore the
music requires no unverified branches of the command dispatcher.

## Replay implementation

The shared replay core now keeps logical asset sizes separate from its maximum
buffers, so the smaller `FLDFX` data retains its original bounds. Music mode
adds a saved start and live pointer per channel, advances tracks independently,
and loops each sequence without resetting persistent channel state. The same
50 Hz speed counter, PAL 3,546,895 Hz period conversion, volume and pitch
macros, initial/loop sample regions, signed 8-bit sample data, and Amiga stereo
placement remain in use.

The envelope terminal check was also made literal: only descriptor/control bit
7 loops a completed macro. A normal `$E1` pitch macro stops after its fourth
segment, while the forced-loop form resumes at segment 1. The Milestone-72
one-second gameplay-effect hash remains unchanged.

## Verification

The regression suite checks the exact header/table addresses, all four initial
sequence and track pointers, each channel's first period/sample/delay state,
and a deterministic five-second stereo PCM FNV-1a value of `a6af1c4d`. It then
clocks the player for 14,300 VBlanks and confirms that every sequence passes
its `$FF`, returns to its own beginning, and stays active. Observed first-loop
times range from 180.26 seconds (channel 2) to 285.80 seconds (channel 0).

Strict C99 and UndefinedBehaviorSanitizer builds pass the complete core suite.
Normal and sanitized headless hosts also pass smoke runs on all 42 caverns.
SDL2 development headers are not installed in the verification environment,
so the SDL host path is retained but was not rebuilt here.

The supplied 30-second preview is signed 16-bit, 44.1 kHz stereo. Its SHA-256
is `07420854d6cfe94b19165bdf69f927342279a499963b84808a66ef147fb721f5`.
