# Flood Milestone 74: Bullfrog intro and title picture

## Result

Milestone 74 reconstructs the first two states of Flood's original startup
sequence: the animated Bullfrog intro at `$B46A-$B932`, followed by the static
`TITLE.SC` presentation picture. Both use their original five bitplanes,
palettes, display dimensions, and disk data.

The user's remembered ordering is supported by the binary:

1. short animated Bullfrog intro;
2. Flood presentation/title picture;
3. level/password selection picture;
4. poster-based copy protection;
5. selected cavern gameplay.

This milestone implements stages 1 and 2. It intentionally leaves the exact
level-code editor and protection challenge for the next startup milestones
rather than inventing substitutes for them.

## Startup call graph

`$96B6` calls `$98A4` during initial setup. That routine allocates the intro
backing, calls music setup `$1647A`, and then enters `$B46A`. Consequently the
`TRACK.DA` music reconstructed in Milestone 73 begins before the Bullfrog
animation.

After `$B46A` returns, file-loader setup `$A612` selects internal descriptor 9,
`TITLE.SCR`. The five-plane Copper path displays it with the palette at
`$1802C`; `$9714-$9724` waits for the fire input. Only then does `$9734` call
the audio shutdown at `$164E4`. The SDL integration now observes those same
music boundaries for the two reconstructed stages.

## Bullfrog intro

Internal file descriptor 10 is `BIG_PIC.SCR`, corresponding to disk file
`BIG_PIC.`. It is an uncompressed 100,000-byte image: five 20,000-byte planes,
each 320x500 at 40 bytes per row. `$B8E4` derives a sixth mask plane as the
bitwise inverse of the OR of the five image planes.

The Copper setup at `$B64E` selects five planes and the 32-word palette at
`$B60E`. This proves the earlier palette correction more specifically:
`$B60E` is real and correct for the Bullfrog presentation, but it is not the
cavern palette.

`$B934` contains 204 sixteen-byte records followed by a `$FFFF` terminator.
Each record is eight big-endian words:

| Word | Meaning |
| ---: | --- |
| 0 | PAL frame |
| 1-2 | source X/Y in the 320x500 backing |
| 3-4 | destination X/Y |
| 5 | width; low bit selects the optimized 160-pixel routine |
| 6 | height |
| 7 | direct copy or mask compositing |

All events scheduled for the current frame execute in table order. On ordinary
frames, `$B870` copies the left 160x76 region at Y=16 to the right half,
creating the mirrored face. Masked records use `(destination & mask) | source`,
matching `$B798-$B79E` and `$B834-$B858`.

The event at `$04C0` is the last one. The automatic run therefore finishes on
PAL frame 1216, exactly 24.32 seconds at 50 Hz. The input test at `$B4C4-$B4CA`
also permits immediate fire-button skipping.

## Title picture

`TITLE.SC` has packed size 34,143 bytes and expands through `$1499C` to 52,800
bytes. The result is five 10,560-byte planes: 352x240 at 44 bytes per row. The
visible presentation is the central 320-pixel crop, X=16 through X=335, using
the 32 colours at `$1802C`. Natural ascending plane significance produces the
coherent Flood title artwork; no plane permutation or palette substitution is
required.

| Extracted data | Bytes | SHA-256 |
| --- | ---: | --- |
| `BIG_PIC.bin` | 100,000 | `e4565689c0922badd13f83a49043e6585ab7849ccea9fee47d9e00da081a18ab` |
| `INTRO_events.bin` | 3,280 | `ac735e60adb0bae4bc565fa47a2a9d60e8b6d1505f9b750dc746a8e5e023dcd1` |
| `INTRO_palette.bin` | 64 | `776257e49e6422eca6d8cbde45bf443a4194c21bee1b97a07744c76f695bca78` |
| `TITLE_SCR.bin` | 52,800 | `a813911faebf53ed81e89b9608fb92030d7daa9eb487656b9b63983d359d7a60` |
| `TITLE_palette.bin` | 64 | `43bc0c20a8b0ff29dc0973e51e1d461cf2435e9d84a6d137df0271249e4d4df5` |

The same extraction pass also preserves `TEMPFILE.SCR` and `HSCORE.DAT` for
the following level-code stage.

## SDL integration and verification

The SDL startup path now opens the original music before the intro, displays
the 320x96 animation at 50 Hz, switches to the 320x240 title picture, waits for
a fresh fire press, and then changes the audio object to the gameplay `FLDFX`
set. Because stages 3 and 4 are not reconstructed yet, the current host then
continues with the level selected on its command line; this boundary is
explicit rather than represented as original behavior.

Regression hashes cover the intro's initial state and frames 1, 240, 320, 512,
and 1216. They also verify event-table progress, automatic completion, the skip
path, both palettes, and the title picture's complete visible 320x240 index
image (`33ed4be6` FNV-1a).

The complete suite passes strict C99 and UndefinedBehaviorSanitizer. Normal and
sanitized headless hosts pass all 42 cavern smoke runs. SDL2 development files
are unavailable in the verification environment, so the guarded SDL code was
reviewed but not linked locally.
