# Flood (Amiga) reverse-engineering notes — first stage

Source image: `flood.ipf`
Recovered logical disk: `flood_880k.adf`

## Custom disk layout

The disk uses 160 logical Amiga tracks (80 cylinders x 2 sides), each containing 11 x 512-byte sectors.

- Track 0: custom boot code (valid AmigaDOS-style boot block marker)
- Track 1: Flood custom directory (`DIR\0`)
- Tracks 2-159: contiguous game files described by that directory

The custom directory records are 16 bytes each:

```
char name[8];
uint16_t start_track;   // big endian
uint16_t end_track;     // inclusive
uint32_t byte_size;     // big endian
```

## RUN_PROG

Directory entry:

- Name: `RUN_PROG`
- Tracks: 2-12
- Size: 60,632 bytes (`0xECD8`)
- Capacity of allocated tracks: 61,952 bytes

The boot loader explicitly references the string `RUN_PROG`, chooses destination address `$00009530`, reads/copies the file data, and finishes with:

```asm
JMP     $00009536
```

Therefore:

- `RUN_PROG` load base = `$9530`
- executable entry point = `$9536`
- file entry offset = `+6`

The first six bytes of `RUN_PROG` are another absolute jump:

```asm
$9530:  JMP     $00017536
```

This is not the entry used by the disk boot path; boot enters at `$9536`.

## Entry wrapper at $9536

Raw bytes and decoded intent:

```asm
$9536:  MOVEM.L <most registers>,-(SP)
$953A:  MOVEA.L $00000004,A6       ; ExecBase
$9540:  MOVE.W  #$7FFF,$DFF09A     ; INTENA: clear interrupt enables
$9548:  MOVE.W  #$E028,$DFF09A     ; enable selected interrupts/master
$9550:  MOVEM.L ...
$9554:  LEA     $000781F4,A0
$955A:  CLR.L   (A0)
$955C:  MOVEM.L ...
$9560:  JSR     $000096B6          ; substantial game init / main control
$9566:  JSR     $00009572          ; currently just RTS (stub)
$956C:  MOVEM.L (SP)+,...
$9570:  RTS
$9572:  RTS
```

`$96B6` is the first substantial routine reached from the boot handoff.

## Beginning of routine $96B6

The routine starts with a stack frame and then performs a sequence of initialization calls:

```asm
$96B6:  LINK    A5,#-12
        MOVE.W  #1,$0007C6B0
        BSR     $000098A4
        BSR     $00009902
        BSR     $00009EE2
        BSR     $0000A4F0
        JSR     $0000E10E
        BSR     $0000A610
        MOVE.W  #5,$00017E6C
        ...
        JSR     $0000EE4A
        ...
```

This is consistent with `RUN_PROG` being the main game executable rather than a tiny secondary loader. It contains strings including `graphics.library`, many game-related data tables, and extensive 68000 code.

## Important correction to the initial disk hypothesis

The IPF does not require reconstructing a standard AmigaDOS filesystem to get the game files. Flood uses its own compact track directory on logical track 1 and stores each named file in contiguous logical tracks. This is simpler and more deterministic for reverse engineering.

## Extracted file table

See `flood_directory.txt` and the `flood_files/` directory. There are 27 entries, including:

- RUN_PROG
- TRACK.DA
- INSTR.DA
- BIG_PIC.
- SPR_16.B
- SPR_32.B
- BLOCKA.B / BLOCKB.B / BLOCKC.B
- W_BLOCKS
- MAP001.D ... MAP041.D
- TITLE.SC
- HSCORE.D
- TEMPFILE
- FLDFX.DU
- FLDFX.IN
- END_SCRE

## Next reverse-engineering milestone

1. Build a reliable 68000 disassembly of `RUN_PROG` using load base `$9530`.
2. Seed function labels from the verified boot call chain (`$9536`, `$96B6`, etc.).
3. Identify OS/library calls and Amiga custom-chip register accesses.
4. Identify file-loading routines and map references to the custom directory files.
5. Partition code into subsystems: startup, disk/file IO, graphics/blitter, input, audio, level/gameplay logic.
6. Translate one subsystem at a time into C, while preserving original integer widths and memory layout.
