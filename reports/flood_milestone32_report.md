# Flood reverse engineering — Milestone 32: Aunt Matilda verified

## Result

The delayed Quiffy-history subsystem at `$CED8-$D002` is now verified as the implementation of **the ghost of Aunt Matilda**.

This is no longer based only on the external gameplay description. The binary itself proves all of the defining behaviors:

1. Quiffy's `(x,y)` position is recorded in a 256-entry circular history.
2. An older history entry is drawn using a Quiffy-derived sprite frame.
3. The old history entry is collision-tested against Quiffy's current position.
4. Contact subtracts exactly **10 Life Force**.
5. The delayed reader periodically advances an extra history sample, making the follower slightly faster than the writer.
6. The extra catch-up stops at an eight-sample minimum lag.

Those properties match Aunt Matilda's documented behavior: copy Quiffy's previous path, begin well behind him, move slightly faster, eventually catch him, and cause damage on contact.

## Memory map

| Address | Meaning |
|---|---|
| `$56036` | 256 `(x,y)` history samples, 4 bytes each |
| `$55F36` | 256 recorded frame bytes |
| `$E1F8` | history write byte offset |
| `$E1F6` | Aunt Matilda history read byte offset |
| `$E1B4` | temporary Matilda read-hold / extra-lag countdown |
| `$E1D0` | Quiffy Life Force |
| `$180AC/$180AE` | current Quiffy X/Y |

## How the long delay works

Initialization clears all 256 coordinate samples and sets:

```text
write offset = 0
read offset  = 4
```

Each update advances the writer first, but Aunt Matilda reads the destination slot **before Quiffy's current position overwrites it**.

For the first complete ring traversal those entries are still zero, so no ghost is drawn. After the ring wraps, the same read slots contain Quiffy's positions from one complete 256-sample cycle earlier.

This is a very economical delay line: no timestamps and no per-sample age values are needed.

## Matilda is slightly faster

After the ordinary reader update, the code checks when the read offset lands on a 128-byte boundary (one boundary per 32 history entries).

Unless the reader is already eight samples behind the writer, it advances one **additional** history entry.

Conceptually:

```c
reader += 1;

if ((reader % 32) == 0 && lag > 8)
    reader += 1;
```

Therefore Matilda consumes Quiffy's path slightly faster than Quiffy produces it. Over a long enough level she steadily closes the gap.

## Contact and damage

The delayed sample is tested as a 16x16 box with the same `+8,+8` center offset used by other player collision code:

```c
if (overlap16(matilda_x + 8, matilda_y + 8,
              quiffy_x + 8, quiffy_y + 8)) {
    life_force -= 10;
}
```

The hit is therefore finite damage, not instant death.

## Rendering

The delayed coordinates are rendered through the normal masked sprite blitter `$E4AA` with sprite base `$D2` plus a recorded Quiffy animation byte.

An important refinement to Milestone 16: the history frame byte is not the full `$D4F2` selector. At `$CFEE-$CFFE` the game records:

```c
frame = (animation_phase & 3) + horizontal_animation_bank;
```

so Aunt Matilda reuses the basic Quiffy horizontal animation frames while replaying his historical coordinates.

## Death/respawn interaction

When Quiffy dies, `$D83C` sets `$E1B4 = 20`. While this countdown is nonzero, Matilda's history reader does not advance, while Quiffy's writer continues. This adds twenty samples back onto her lag, effectively giving the player extra separation around the death/respawn sequence.

## External cross-check

Contemporary/reference descriptions say Aunt Matilda follows Quiffy's movements with a substantial delay, moves very slightly faster, eventually catches him, and hurts him on contact. The recovered implementation independently exhibits every one of those traits.

## New source

`src/aunt_matilda.c` provides a portable C reconstruction of the history, catch-up, collision, and draw-command logic. It compiles with `-Wall -Wextra -Werror`.

## Next useful target

The Matilda subsystem is now sufficiently complete to stop treating it as an unknown player-history effect. The next high-value reverse-engineering work can return to the remaining static-object/state-23 mysteries, while integrating Aunt Matilda into the growing reconstructed engine loop.
