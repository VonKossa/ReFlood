# Flood reverse engineering — Milestone 16

## Scope

This milestone connects the player animation selectors from `$D460/$D4F2` to the actual sprite IDs used by the masked sprite renderer and analyzes the player-history/overlay portion of `$CE80-$D070`.

## High-confidence findings

### Player selector is relative to sprite base `$D2`

The frame selector returned by `$D4F2` is stored as a byte in the table at `$55F36`. During rendering, the code reads that byte, pushes `$00D2`, adds the selector to the stack argument, and calls `$E4AA`.

Therefore:

```c
quiffy_sprite_id = 0xD2 + frame_selector;
```

This turns the numeric selector banks from milestone 15 into concrete sprite-space formulas.

### Concrete movement sprite formulas

- floor/south surface: `0xD2 + 2*horizontal_bank + phase`
- ceiling/north surface: `0xDA + 2*horizontal_bank + phase`
- west wall: `0xE2 + 2*vertical_bank + phase`
- east wall: `0xEA + 2*vertical_bank + phase`
- water/airborne: `0xF2 + 2*horizontal_bank + phase`
- special floor/down-input path: `0x11E + 2*horizontal_bank + phase`
- diagonal corner-transition selectors `$55-$5D` map to sprites `$127-$12F`

Because the bank fields are generally 0 or 4, the sprite ranges intentionally overlap. The layout appears designed for smooth orientation changes rather than as isolated animation strips.

### 256-entry player frame/position history

The renderer maintains two parallel history arrays:

- `$56036`: 1024-byte coordinate ring, 256 entries × `(x,y)` words
- `$55F36`: 256-byte frame-selector ring

`$E1F8` is the write offset and `$E1F6` the read offset. Both are byte offsets into the four-byte coordinate entries and wrap with `& 0x03FF`.

Every recorded sample contains the current player position plus the current `$D4F2` selector. A draw sample is converted to a real sprite ID by adding `$D2` and passed to `$E4AA`.

### Death path deliberately stalls history consumption

Normal operation advances the read pointer by one entry. If `$E1B4` is nonzero, the renderer decrements `$E1B4` instead and leaves the read pointer stationary.

The death-state code at `$D83C` sets `$E1B4 = 20`. The writer continues to move while the reader is held, producing a deliberate delayed-history effect. The exact visual name (afterimage/replay/etc.) is not assigned without observing the original animation, but the mechanism is now reconstructed.

### Two timed player overlays

The render block also owns two power-up/status timers:

- `$E1B6`: when nonzero, draw sprite `$C6` at player Y − 24 and decrement
- `$E1BA`: when nonzero, draw sprite `$C7` at player Y − 24 and decrement

Pickup handlers around `$FCEE/$FD10` set one timer to 1000 and clear the other. They are real gameplay power-ups with separate movement effects; names remain provisional until the corresponding pickup art/table entries are identified.

## Correction to the previous target description

`$CE80-$D070` is not a simple immediate 'current sprite lookup'. It is a broader player presentation routine containing frame selection, a position/frame history ring, delayed-history drawing, collision/damage attached to one presentation object, and timed overlay sprites.

The important sprite-ID bridge is nevertheless exact: player animation selector + `$D2`.

## New reusable code

`src/player_render_history.c` implements the recovered history ring and selector-to-sprite conversion without Amiga hardware dependencies.

## Recommended next target

The best next target is the pair of pickup/status paths around `$FCEE-$FD24` and the physics consumers around `$DCC6-$DD40`. Those should identify what sprites `$C6/$C7` represent and give names to `$E1B6/$E1BA`, completing two currently anonymous player power-ups.
