# Flood Milestone 77: exact high-score board and name editor

## Result

Milestone 77 reconstructs the game-over high-score path at
`$12A38-$12DA0`. The SDL host now opens the original five-entry board after
Quiffy loses the last life, inserts a qualifying score, runs the joystick/fire
name editor, and exits on a fresh fire press and release.

The score table is deliberately session memory only. The original executable
contains no score-file write path: `HSCORE.DAT` is the editor's glyph artwork,
not persistent score data.

## Initial table and insertion

The executable initializes five big-endian score longs at `$17E94` and five
12-byte names at `$17EA8`:

| Rank | Score | Stored name |
| ---: | ---: | :--- |
| 1 | 2000 | `SEANY======` |
| 2 | 1800 | `GLENN======` |
| 3 | 900 | `KEVIN======` |
| 4 | 400 | `SIMON======` |
| 5 | 100 | `PETER======` |

The comparison is signed and accepts equality. A qualifying score shifts lower
records down, replaces the selected name with 11 `=` characters plus NUL, and
drops the former fifth entry. Scores are drawn as fixed five-digit decimals.

## Display reconstruction

The board reuses the four-plane, 352x240 `TEMPFILE` backing and displays its
central 320-pixel crop with the presentation palette at `$17FEC`. Rows begin at
visible Y=20 and are 32 lines apart. Names begin at visible X=0; scores begin at
X=208.

`HSCORE.DAT` expands to 10,320 bytes: 43 records of 240 bytes. Each record is
24 rows of a mask word followed by four plane words. The static board compositor
uses source offsets `2,4,8,6`, preserving the original interchange of its final
two source planes. The editor's final-character helper uses natural order
`2,4,6,8`; Milestone 77 retains this observable difference rather than
normalizing the colours.

The 43 glyph indices map consecutively to ASCII `$30-$5A`. Thus index 0 is
`0`, index 10 is `:`, and index 42 is `Z`.

## Exact editor behavior

The editor has 11 positions. Up and Down move toward the adjacent glyph by
240 bytes, but the visible transition advances only 20 bytes per PAL frame.
This takes 12 frames and exposes the original vertically scrolling strip.
Selection is bounded to the 43 recovered glyphs.

Left or Right first commits the active character, then moves the cursor within
positions 0-10 and loads the destination character's glyph. Another horizontal
move is not accepted until the joystick returns to neutral.

Fire acts only when glyph scrolling has settled. It waits for release, commits
the selected character, copies it into the next position as the initial editor
state, and advances. At position 10 it completes the name. Selecting `:` at any
earlier position terminates immediately and fills the remaining stored bytes
with `=`. The displayed colon remains visible until the board is dismissed.

Nonqualifying scores skip the editor. Both paths leave the board only after a
fresh fire press/release. The original's very long unattended timeout is not
needed by the interactive host.

## Extracted assets

| File | Bytes | SHA-256 |
| :--- | ---: | :--- |
| `HIGHSCORE_scores.bin` | 20 | `44a10c88c7b47da56f2f1a570b2cfc2ecb5d19dfcf6ec81136d61e86826fdbb3` |
| `HIGHSCORE_names.bin` | 60 | `558b50c965350c40aa402cc1af1a48f09752f5759ab8b94fd663bad94d4c5e7e` |
| `HSCORE_DAT.bin` | 10,320 | `7d0f187857d8a57aaf3065a921167be22d83034afcb7e1c61b1152962cbdd825` |
| `TEMPFILE_SCR.bin` | 42,240 | `61aaa05934149bae6dd2d9179d6621c5c0ab7ca9edc3001242833ec2675dc7b4` |
| `LEVEL_palette.bin` | 32 | `64690c1ca824a1d25f0fd8fdabdcebf2d14c5f32a7e288d4ab400dedbbc49115` |

## Verification

Regression tests pin the initial records and palette, insertion at rank three,
untouched and active board pixel hashes, a half-complete glyph transition,
horizontal navigation, the complete `COD:` early-termination path, memory
padding, and the final fire-release handshake. The preview is produced by the
C implementation used by the host, not by a separate visual approximation.
The complete suite passes strict C99 and UndefinedBehaviorSanitizer builds;
normal and sanitized headless hosts also complete smoke runs on all 42 caverns.

See `high_score_editor_milestone77.png` for the untouched board, inserted score,
mid-scroll animation, and completed-name states.
