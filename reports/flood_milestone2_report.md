# Flood reverse engineering - Milestone 2

## Scope

This milestone starts from the recovered `RUN_PROG` loaded at `$00009530`, with the boot loader's verified jump to `$00009536`.

The goal was to establish a trustworthy control-flow map before attempting broad assembly-to-C translation. All symbolic names below are reconstructed labels; they are not original Bullfrog names unless explicitly stated.

## 1. Verified entry path

The game entry at `$9536`:

- saves D0-D7/A0-A6;
- loads ExecBase from absolute address `$4`;
- writes to `INTENA` (`$DFF09A`), temporarily taking control of interrupt configuration;
- clears a longword at `$781F4`;
- calls `$96B6` (now labelled `MainControl`);
- calls `$9572` on return;
- restores registers and returns.

A second routine at `$9574` explicitly opens `graphics.library` using Exec `OldOpenLibrary` (`jsr -408(a6)`). It obtains the system copper-list pointer from the graphics library and writes it to `COP1LC` (`$DFF080`), strobes `COPJMP1` (`$DFF088`), and writes `DMACON` (`$DFF096`). This is strong evidence that this routine restores/re-establishes display hardware state around the game's takeover of the machine.

## 2. `$96B6` is the main high-level control routine

`$96B6` begins with `LINK A5,#-12` and immediately calls six initialization routines:

- `$98A4`
- `$9902`
- `$9EE6`
- `$A4F2`
- `$E10E`
- `$A612`

It then prepares six stack arguments and calls `$EE4A`, waits on global `$17E5A` while repeatedly calling `$13E06`, establishes state values at `$17E86` / `$180CC`, and enters further state-driven loops.

The routine repeatedly checks signed values in `$17E80`. Three sentinel values are visible:

- `-2` (`$FFFE`)
- `-3` (`$FFFD`)
- `-20` (`$FFEC`)

`-20` is the exit condition for the outer runtime loop. `-3` selects an additional transition path that includes `$15AD2`. This makes `$17E80` a strong candidate for a high-level game/mode transition variable.

## 3. Input routine identified with high confidence: `$13E06`

`$13E06` is called by the early wait loop and directly reads Amiga input hardware:

- `$BFEC01` - CIAA serial data register, used by the keyboard path;
- `$DFF00C` - joystick data register;
- `$BFE0FF` / CIA-related address;
- `$DFF016` and `$DFF034` - custom-chip input/control registers.

It updates globals around `$17E5A`, `$17E5C`, `$17E78`, `$17E7A`, and `$17F2A`.

For the reconstructed source tree we can therefore label `$13E06` as something like `PollInput` / `ReadControls` with high confidence, while keeping the exact semantics of each global provisional.

## 4. Graphics/blitter cluster identified

A dense hardware-access region begins around `$E4AA` and includes writes to:

- `$DFF040` BLTCON0
- `$DFF042` BLTCON1
- `$DFF044` BLTAFWM/BLTALWM region
- `$DFF048`, `$DFF04C`, `$DFF050`, `$DFF054` blitter pointers
- `$DFF058` BLTSIZE
- `$DFF060`, `$DFF062`, `$DFF064`, `$DFF066` blitter modulos
- `$DFF002` DMACONR (busy polling)

The code explicitly polls bit 6 of `$DFF002` before programming the blitter. This cluster is therefore confidently graphics/blitter code. `$E4AA` is called many times from the game and is a prime candidate for the central sprite/tile/object blit routine.

## 5. Display / interrupt setup cluster

Around `$1572A` the program writes:

- `$DFF09E` ADKCON
- `$DFF07E`
- `$DFF096` DMACON
- `$DFF020` / `$DFF024`
- `$DFF09A` INTENA

and calls several nearby routines. This is likely display/copper/interrupt initialization rather than gameplay logic.

## 6. Audio cluster identified

Around `$17086`, code uses A1 = `$DFF0A0`, the base of Amiga audio channel 0 registers, and writes channel registers plus `DMACON`. This identifies the `$170xx` region as audio replay / channel-control code.

## 7. Static memory layout initialization

`$9902` writes a set of fixed pointers into globals:

- `$17F0A = $4F8A0`
- `$17F0E = $4F8A0 + $3200`
- `$17F12 = $4F8A0 + $3210`
- `$17F1A = $5E436`
- `$17F1E = $56436`
- `$17F22 = $10ED0`
- `$17F16 = $52D34`
- `$17F26 = $5E538`

It also clears/initializes state at `$17E80`, `$17E90`, `$17F2A`, and `$180D0`. This is a valuable anchor for reconstructing the original memory map and eventual C structures.

## 8. `$98A4` prepares dynamic/global buffers

`$98A4` clears two state words, calls `$FB02`, invokes `$FAC2` twice with the same two immediate stack arguments (`2`, `$190`), and stores the returned pointers in `$7C6DE` and `$7C6E2`. It also installs `$1F45C` at `$7C6EE` and calls `$1647A` / `$B46A`.

The exact role of `$FAC2` is not yet named; it behaves like a helper returning a pointer, but it would be premature to call it an allocator until its implementation is lifted.

## 9. Hardware footprint scan

A syntactic scan of `RUN_PROG` found repeated references to Amiga custom registers. Most frequent include `$DFF080` (COP1LC), `$DFF096` (DMACON), `$DFF002` (DMACONR), `$DFF054/$DFF058` (blitter pointer/size), `$DFF09A` (INTENA), and the audio register base `$DFF0A0`.

These references give us natural subsystem boundaries for the next lifting passes.

## 10. Candidate call map

A conservative byte-pattern scan found 796 syntactic JSR/BSR candidates. The early control-flow slice is especially reliable because it starts at a verified function entry and its targets align with clear prologues / code structures.

The accompanying `flood_candidate_calls.csv` is intentionally labelled *candidate*: broad byte scans can produce false positives in embedded data. It is useful for prioritisation, not as a final control-flow graph.

## 11. First C lift

`flood_maincontrol_pseudocode.c` is the first C-shaped reconstruction of `$96B6`. It deliberately retains raw globals and anonymous `sub_xxxxxx()` calls. The next goal is to replace those with typed state and named subsystems only after their semantics are verified.

## Recommended next milestone

1. Fully lift `$13E06` into C and model keyboard/joystick state.
2. Lift `$E4AA` and its immediate callees into a blitter abstraction.
3. Lift `$170xx` audio channel routine enough to identify sample data structures.
4. Trace `$EE4A` to determine what the six stack arguments represent.
5. Build a symbol table and memory-map document from globals touched by those routines.

This route should give us the first genuinely compilable C subsystem (input) before tackling the more complex graphics/gameplay code.
