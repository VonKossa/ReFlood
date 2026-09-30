# Flood Milestone 76: exact poster copy protection

## Result

Milestone 76 reconstructs the poster-based copy-protection state machine at
`$A940-$ACB8` and inserts it after the level-code selector and before gameplay.
It uses the original four language flags, selection marker, gameplay palette,
font, encrypted challenge table, answer comparison, localized messages, and
two-failure limit.

The resulting host startup order is now:

1. short Bullfrog intro;
2. Flood title picture;
3. level/password selector;
4. poster copy protection;
5. selected cavern gameplay.

## Display reconstruction

The protection stage uses the 320x208 gameplay display with a black background
and the 16-colour palette at `$17F2C`. The language symbols are ordinary game
sprites drawn by `$E4AA`:

| Language | Four 16x16 sprite IDs | X range | Marker X |
| :--- | :--- | ---: | ---: |
| English | `$00-$03` | 64-95 | 72 |
| German | `$04-$07` | 112-143 | 120 |
| French | `$08-$0B` | 160-191 | 168 |
| Italian | `$4C-$4F` | 208-239 | 216 |

All flags occupy Y=32-63. Global 32x32 sprite `$50` is drawn at Y=24 over the
selected language. Text uses `$14DBA` and the same `$149FA` font recovered for
the selector. Colour 15 is copied into all four bitplanes.

The text placements recovered from calls to `$9E6A` are:

| Content | Character X | Character Y |
| :--- | ---: | ---: |
| localized “From the poster” | 2 | 10 |
| challenge | 2 | 12 |
| `>` prompt | 2 | 14 |
| typed answer | 3 | 14 |
| correct/wrong message | 2 | 16 |
| retry “Press fire” message | 2 | 18 |

Strings wrap after character column 41 and continue at column 2 on the next
row, exactly as `$9E6A-$9EC4` specifies.

## Challenge data

`$10EA8` XORs the `$1720` bytes at `$10ED0-$125EF` with `$AA` in place. The
result is four language banks of 20 records. Each record is exactly 74 bytes:

- bytes 0-60: NUL-terminated, space-padded question;
- bytes 61-73: exact 13-byte, space-padded answer.

The random generator at `$D77C` advances once after language confirmation.
The positive 15-bit result is divided by 19, so only record indices 0-18 are
reachable. Record 19 exists in all four banks but is never selected.

### Reachable English challenges

| Index | Challenge subject | Exact answer |
| ---: | :--- | :--- |
| 0 | Balloons | `6` |
| 1 | K characters on cassette | `64` |
| 2 | Straws in cocktail | `2` |
| 3 | Drink brand | `COLA` |
| 4 | Teddy teeth | `10` |
| 5 | Drips from tap | `5` |
| 6 | Vines on tombstone | `3` |
| 7 | Missile stripes | `4` |
| 8 | Stars around ghost | `24` |
| 9 | Writing on disk | `BULLFROG` |
| 10 | Names on credits | `17` |
| 11 | Shuriken points | `4` |
| 12 | Text under Quiffy’s tattoo | `QUIFFY` |
| 13 | Character standing on balloons | `PSYCHO TEDDY` |
| 14 | Character in Quiffy’s goggle | `DOCTOR DUSTY` |
| 15 | Grenades | `2` |
| 16 | Plonkin Donkins | `3` |
| 17 | Character on parachute | `VACUOUS GOMBO` |
| 18 | Dynamite sticks | `8` |

The unreachable index 19 asks about the snail stripes and has answer `10`.
German, French, and Italian banks contain translated questions with the same
13-byte answers.

## Input and state machine

The state word local to `$A7BC` has four values:

| State | Meaning |
| ---: | :--- |
| 1 | language flags and confirmation |
| 2 | challenge entry |
| 3 | correct result |
| 4 | wrong result |

State 1 applies the current signed horizontal joystick value to the language
index and masks it modulo four when fire is pressed. It waits for joystick and
fire release before selecting a question. The host preserves this confirmation
behavior; English remains the cold-start default.

State 2 accepts at most 13 keyboard bytes. Backspace (`$7F`) restores a space,
Return (`$0D`) is ignored at cursor zero, and duplicate held keys are filtered.
Return compares all 13 bytes, including trailing spaces, with the record answer.
SDL converts letters to uppercase and permits Space inside multiword answers.

A correct answer displays the localized success line and waits for a complete
fire press/release. A wrong answer displays both retry lines and does the same.
The first failure clears all 13 positions and retries the same question. The
second failure exits the protection path as failed.

## Extracted assets

| File | Bytes | SHA-256 |
| :--- | ---: | :--- |
| `PROTECTION_palette.bin` | 32 | `9f483f824bc885389a86f5c5ea86076b7939f16f6898440c72a3d8d47cc231c3` |
| `PROTECTION_challenges_xor.bin` | 5,920 | `ae40680bafbb3727bc69207a29297ad14e43e765838a23e14a7ba8987444bc29` |
| `PROTECTION_messages.bin` | 367 | `96bcb1d39de663423b328bb2d4df6f1f73cbace731129a8ff4ed2817f7b40027` |
| `PROTECTION_message_offsets.bin` | 32 | `da97f564ee9ba187bea2c949e953b881539bcea4d1460a159a984307735b989a` |
| `PROTECTION_flags.bin` | 96 | `474033d3456f8beeb528ce2cca29d5641ec129abceb12af2ddd7c2ce18c85c89` |
| `PROTECTION_input.bin` | 14 | `471f9476bbb284f87b3f9cb728bd91cc31b4710ee4a500c0dafad73e5021beb6` |

## Verification

Tests pin the decryption boundary, palette, live and unreachable records,
language wrapping, RNG advancement, initial/question/wrong/correct pixel hashes,
13-byte comparison, correct acknowledgement, first retry, and second-failure
exit. The complete suite passes strict C99, UBSan, and all 42 standalone level
smoke runs.

See `copy_protection_milestone76.png` for four C-rendered states.
