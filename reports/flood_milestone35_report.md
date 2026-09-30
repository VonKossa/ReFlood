# Flood reverse engineering - Milestone 35

## Real Quiffy artwork in the SDL host

The player placeholder rectangle from Milestone 34 has been removed. The host now loads both original sprite banks:

- `SPR_16_unpacked.bin`: 80 masked 16x16 sprites, global IDs `$00-$4F`;
- `SPR_32.B`: 150 masked 32x32 sprites, global IDs `$50-$E5`.

A 32x32 sprite record is 640 bytes:

- 128 bytes shared mask;
- 128 bytes bitplane 0;
- 128 bytes bitplane 1;
- 128 bytes bitplane 2;
- 128 bytes bitplane 3.

Quiffy's basic host frame is now selected as:

```c
sprite_id = 0xD2 + ((animation_phase & 3) + horizontal_bank);
```

This is the mapping recovered from the original renderer. Aunt Matilda also composites the delayed `$D2 + recorded_frame` sprite rather than a debug rectangle.

## Collision integration

The four-corner host collision approximation has been removed.

The engine now exposes the recovered `$F0E0`-style query:

```c
FloodCollisionResult flood_query_collision(
    game, x, y, dx, dy, width, height);
```

It separately produces:

- `x_flags`: attributes along the edge reached by X movement;
- `y_flags`: attributes along the edge reached by Y movement;
- `corner_flags`: the moved diagonal corner.

The engine also uses the recovered `$F50A` mapping:

```c
index = ((y >> 4) * 128) + (x >> 4);
```

Tile attribute bit `$02` remains the high-confidence solid/blocking flag.

The **query itself** is reconstructed from the original routine. Player collision response is still intentionally simpler than the original Quiffy eight-neighbor contact resolver; this milestone uses the query to accept/reject horizontal and vertical movement. The recovered wall/ceiling attachment system will be integrated next.

## Exact water fill lookup restored

Milestone 34 still contained a host interpolation for transient water states. This milestone replaces it with the exact recovered 40-entry lookup from `$109E8`, including zero-valued transition phases and visible fill heights of 1,2,4,6,8,10,12,14,16 pixels.

## Validation

The project was rebuilt from scratch in headless mode with compiler warnings enabled. CTest passes. Additional tests now verify:

- level 1 loads at Quiffy's real start `(96,80)`;
- sprite `$D2` decodes to a non-empty 32x32 masked sprite;
- `$F50A` tile indexing gives index 129 for pixel `(16,16)`;
- the swept collision query detects a synthetic solid tile on the approached X edge.

The headless host also successfully runs level 1 and level 23 through 300 update ticks.

## Next milestone

The highest-value next integration is the already recovered Quiffy contact system:

1. build the eight-neighbor contact mask;
2. restore floor/wall/ceiling attachment selection;
3. use the real animation selector for floor, ceiling, walls, airborne/water, and corner transitions;
4. then seed and render the actual flood-water state in the real cavern.

That should turn the SDL host from a conventional gravity prototype into movement that begins to feel specifically like *Flood*.
