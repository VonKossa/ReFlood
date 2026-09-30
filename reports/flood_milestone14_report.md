# Flood reverse engineering -- Milestone 14

## Result

The central Quiffy movement-state representation is now substantially understood.

### Player core record

```text
$180AC  x
$180AE  y
$180B0  dx
$180B2  dy
$180B4  animation phase / respawn timer
$180B6  horizontal animation-direction bank
$180B8  vertical animation-direction bank
$180BA  animation hold/suppress field (producer still unresolved)
```

### Surface contact model

`$17E68` is an 8-bit neighborhood/contact mask arranged clockwise:

```text
       bit0 N
 bit7 NW     bit1 NE
bit6 W         bit2 E
 bit5 SW     bit3 SE
       bit4 S
```

The construction routine `$F22C` derives this mask from collision probes. `$DAF0` popcounts the eight bits into `$E1D6`.

This explains the wall/ceiling traversal system. `$D9A0-$DAEA` examines the contact ring and joystick direction and chooses a surface attachment:

```text
$E1D8 = 0   no selected attachment axis
$E1D8 = 1   horizontal surface (floor / ceiling)
$E1D8 = 2   vertical surface (wall)

$E1DA        selected attachment X direction
$E1DC        selected attachment Y direction
$E1DE        attachment/release timing state
```

The cardinal contact plus adjacent diagonal bits are handled as a coherent local geometry ring, including corner transitions.

### Death and respawn state

`$E1D4` is now high confidence:

```text
0 = alive / normal
1 = dying / falling
2 = respawning / re-entry animation
```

Mode 1 persists until the floor contact bit (`bit4`) is set. The game then moves Quiffy up 8 pixels, clears the animation phase, changes to mode 2 and plays sound `$38`.

Mode 2 uses `$180B4` as a 16-step timer. At phase 8 it plays `$38` again. At phase 16 it restores:

```text
Life Force = 511
Air        = 63
```

and decrements the lives counter `$17E76`.

### Animation orientation

During normal movement resolution:

```text
resolved dx < 0 -> $180B6 = 4
resolved dx > 0 -> $180B6 = 0
resolved dy < 0 -> $180B8 = 4
resolved dy > 0 -> $180B8 = 0
```

Those values are consumed by the sprite-selection code around `$D460-$D60E`, so they are animation-bank/orientation selectors rather than physical velocities.

## New reconstructed source

- `include/flood_player_state.h`
- `src/player_surface_state.c`
- `src/player_death_respawn.c`

Both C files compile cleanly with `-std=c11 -Wall -Wextra -Werror`.

## Confidence / remaining ambiguity

Very high confidence:

- contact ring geometry;
- `$E1D6` contact count;
- `$E1D4` three-stage lifecycle;
- `$180B6/$180B8` animation-direction banks;
- floor contact = bit 4.

High confidence:

- `$E1D8` as horizontal-vs-vertical attachment axis;
- `$E1DA/$E1DC` as attachment direction vector.

Still unresolved:

- the semantic name of `$180BA` beyond its observed animation-hold effect;
- the complete meaning of `$E1DE` and the exact detach grace-period behavior;
- full sprite-state selection across `$D460-$D60E`.

## Next target

The next high-value block is `$D460-$D60E`, which converts contact geometry, attachment state and animation banks into Quiffy's actual sprite/pose selection. Lifting it should let us name concrete poses such as floor walk, wall climb, ceiling walk, jump/fall, and swim, and connect the movement state machine directly to rendering.
