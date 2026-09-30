# Flood Milestone 75: exact level-code selector

## Result

Milestone 75 reconstructs the startup level/password screen at `$99AC-$9E68`
and places it after the title picture and before the still-unimplemented copy
protection stage. The SDL sequence is now intro, title, level-code selector,
then the current gameplay boundary.

The selector uses the original unpacked `TEMPFILE` picture, the dedicated
16-colour palette at `$17FEC`, the original one-bit font, the six-line panel,
the exact success/failure messages, and the 48-entry password table.

## Exact display path

`TEMPFILE` expands to `$A500` bytes: four consecutive `$2940`-byte planes with
44 bytes per row and 240 rows. Like `TITLE.SC`, its 352-pixel backing is shown
through the central 320-pixel crop. Plane significance is natural ascending
order. The screen palette is the 16-word table at `$17FEC`:

`000 222 444 666 888 AAA CCC EEE C60 A40 820 600 606 828 A4A C6C`

The panel occupies 20 character cells at byte columns 12-31 and character rows
9-14. Each glyph is eight bytes high. The character routine at `$14DBA`
indexes the `$149FA` table with `character - 20`; here 20 is decimal (`$14`),
not the ASCII space value `$20`. All four visible planes receive the glyph
byte, reproducing both set and cleared pixels exactly.

## Password table

| Level | Code | Level | Code | Level | Code |
| ---: | :---: | ---: | :---: | ---: | :---: |
| 1 | FROG | 17 | ZING | 33 | GOGO |
| 2 | YEAR | 18 | JING | 34 | LETS |
| 3 | QUIF | 19 | LIDO | 35 | QUAD |
| 4 | LONG | 20 | POOL | 36 | BRIL |
| 5 | WORD | 21 | HATE | 37 | EGGS |
| 6 | FRED | 22 | REED | 38 | HENS |
| 7 | WINE | 23 | LIME | 39 | NAIL |
| 8 | GRIP | 24 | QUID | 40 | SOAP |
| 9 | TRAP | 25 | WING | 41 | FOAM |
| 10 | THUD | 26 | FLEE | 42 | MEEK |
| 11 | FRAK | 27 | GIGA | 43 | ???? |
| 12 | VINE | 28 | HEAD | 44 | ???? |
| 13 | JUMP | 29 | LOOP | 45 | ???? |
| 14 | NILL | 30 | SING | 46 | ???? |
| 15 | FOUR | 31 | JOUX | 47 | ???? |
| 16 | GRIT | 32 | PINK | 48 | ???? |

## Editor semantics

- Input is retained as four positions, with unused positions redrawn as `?`.
- Delete/backspace (`$7F`) removes one position.
- Return (`$0D`) does nothing when no characters have been entered.
- Return after four characters scans all 48 table entries.
- A match selects the corresponding one-based level and displays
  `#     CORRECT      #` for 130 PAL frames.
- A mismatch or incomplete entry resets the password to `????`, selects level
  1, and displays `#    INCORRECT     #` for 130 PAL frames.
- Fire leaves the selector with the currently selected level.

The original duplicate-key suppression byte is retained. SDL letter input is
normalized to uppercase; Return and Backspace/Delete map to the recovered key
codes. Space remains the fire/continue control.

## Startup/audio integration

The presentation music loaded before the intro continues through the title.
It is stopped at the recovered post-title boundary before the silent selector.
After selector exit, the chosen cavern is loaded and `FLDFX` replaces the
presentation player. Copy protection remains the next startup stage to recover.

## Extracted assets

| File | Bytes | SHA-256 |
| --- | ---: | :--- |
| `LEVEL_palette.bin` | 32 | `64690c1ca824a1d25f0fd8fdabdcebf2d14c5f32a7e288d4ab400dedbbc49115` |
| `LEVEL_template.bin` | 126 | `4375ca20ca0caff50599cd9a7cafa9549a0c1ce0f91b44a1d711338f7352cefb` |
| `LEVEL_correct.bin` | 20 | `17d4e83a7c0ab47c7ceb0f480f4cc78cb85e9752ce3e072eb7c31fd7d45c10f0` |
| `LEVEL_incorrect.bin` | 20 | `ad851d14a9a2f198abd29296872d61c901665afa1ecaac44c56186e8e148edf0` |
| `LEVEL_font.bin` | 960 | `8081f618f4f36b797e1e2d68812a9f0f7a94f0c94066e14f6e32c4a5c9a6c4a4` |
| `LEVEL_passwords.bin` | 192 | `795b9c1fb4e3f8e0e2dd50b5740d484a45361d47a641255c1123d266b85fadb6` |

`TEMPFILE_SCR.bin` remains the disk-derived 42,240-byte background extracted
in Milestone 74.

## Verification

Regression tests pin the palette, representative password entries, initial,
partially typed, accepted, and rejected screen hashes, selected-level changes,
status timing, and fire exit. The entire suite builds under strict C99 warnings,
runs under UBSan, and completes a standalone smoke run for every shipped level.

See `level_code_selector_milestone75.png` for four exact rendered states.
