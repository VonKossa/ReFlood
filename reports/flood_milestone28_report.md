# Flood reverse engineering — Milestone 28

## Main result: the 10-byte trigger language is decoded

The trigger processor at `$12DA6` reads the active-record count from the level control block, then scans 10-byte records. The first word is compared with the current event selector (`$12DA2`). On a match, the remaining four words are passed to `$12E12`.

The recovered record layout is:

```c
typedef struct FloodTriggerRecord {
    uint16_t event_id;
    uint16_t map_offset;
    uint16_t width;
    uint16_t height;
    uint16_t tile_or_marker;
} FloodTriggerRecord;
```

`$12E12` writes `tile_or_marker` into a `width × height` rectangle beginning at `map_offset`, using the map's 128-byte row stride. For changed cells, `$12E48` checks whether the new byte is a special level marker and, when applicable, invokes the same marker dispatcher at `$E7B0` used during level initialization.

This means Flood's level triggers are a compact **event → map mutation/spawn language**. A switch can replace terrain, open/close a structure, or dynamically create an object merely by writing one of the special marker bytes into the map.

## Corrected interpretation of `triggers_words`

The old manifest exposed each record as five anonymous words. The final word is **not a command opcode**. It is the tile/marker value to paint. For example:

```text
[280, 3755, 1, 1, 20]
```

means: when event 280 fires, write marker 20 at map offset 3755. Marker 20 is the horizontal bolt-launcher initializer, so this trigger dynamically creates that mechanism.

A record such as:

```text
[1677, 514, 1, 6, 30]
```

writes ordinary tile `$1E` into a 1×6 rectangle instead of spawning an object.

## Marker language is now a level-object language

The high-confidence marker mapping currently includes:

- `1` → Doctor Dusty
- `2` → Snail (state 12, subtype `$66`)
- `5` → Beady Ball
- `8` → Vacuous Gombo
- `12` → Lumpy Wanderer (state 12, subtype `$30`)
- `14` → Plonkin Donkin
- `16/17/18` → shared mechanical/portal-style mechanism family
- `19` → unresolved runtime mechanism
- `20` → horizontal bolt launcher (runtime state 22)
- `21` → level/flood setup
- `22` → Quiffy start position
- `23` → level/camera-bounds setup
- `24` → Bulbous Headed Vong (state 12, subtype `$6E`)
- `25` → Psycho Teddy (state 12, subtype `$38`)

`marker_language.csv` gives occurrence counts and level lists for all 0–25 values.

## Stronger state-23 conclusion

This pass closes another plausible route to the mysterious weapon-hittable runtime chain `23 → 24 → 25 → 26`.

Triggers **can** dynamically write marker 23, but marker 23's initializer does not allocate a runtime object. It updates level/camera-bound globals. Therefore neither:

1. normal level initialization, nor
2. the decoded trigger-driven marker system

can directly produce runtime behavior 23 through `marker 23`.

The three-hit object must therefore be created by a different runtime transformation/copy path. The WinUAE value-matched watchpoint from Milestone 26 remains the most direct way to catch that exact write.

## Sparkling Fungi status

The trigger/marker model is useful for the Fungi hunt because it tells us which static-looking hazards are actually runtime objects and which are pure terrain mutations. None of the already-named special markers has the required combination of stationary + instant lethal contact. The remaining high-value candidates are marker/state 11, the unresolved state-19 mechanism, and ordinary tile hazards whose lethality is handled outside the standard terrain-attribute bits.

## New artifacts

- `marker_language.csv` — marker counts, levels and current semantic names.
- `triggers_decoded.csv` — all trigger records using the recovered field names.
- `triggered_markers.md` — special markers that are created dynamically by triggers.
- `flood_trigger_format.h` — portable C record definition.

## Next target

Decode the unresolved marker/state-11 and marker/state-19 mechanisms and inspect their corrected sprite/tile art. In parallel, a single WinUAE hit on the state-23 watchpoint should identify the three-hit destructible's creator immediately.
