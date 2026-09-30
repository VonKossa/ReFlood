# Flood reverse engineering -- Milestone 10

## Result

The `$F630/$F698` cluster is the game's **flood/water propagation engine**, not a generic terrain-redraw path.

### Water-state grid

`$17F16` is a 128x100 byte grid (12,800 bytes). This size is independently confirmed by `$F55A`, which clears exactly 12,800 bytes. `$F596` later masks the same 12,800-byte region with `0x3f`, showing that the grid stores compact per-cell state rather than graphics.

### `$F630`: scheduler scan

`$F630` scans the row selected by `$55F34`, 128 cells wide. It ignores state bytes `<= 0`, ignores values `>= 40`, and chooses the cell containing the **smallest positive state**. If the row contains no candidate, it decrements the scan row and tries again. It returns the selected byte index into `$17F16` or zero when no candidate remains.

This is now lifted to strict, host-compilable C in `src/water_scan_f630.c`.

### `$F698`: state dispatcher

`$F698(index)` loads `water_state[index]` and dispatches through a 40-entry jump table. The handlers mutate nearby cells, call direction-specific neighbour predicates, and write new small state values into the water grid.

This establishes a cellular/state-machine flood simulation. The exact semantic label for every transient state (`1..39`) remains open; I have deliberately not invented names for them.

### Tile attributes gain a second meaning

The neighbour predicates (`$F842`, `$F87E`, `$F8C6`, `$F90C`, `$F952`, `$F99A`) consult the normal terrain tile map at `$17F0A` and tile-attribute table at `$17F1A`.

Recovered attribute bits now are:

- `0x01`: blocks **water propagation**.
- `0x02`: blocks **creature/player movement** (from milestones 6/9).

This means Flood's tile metadata deliberately separates water permeability from ordinary solid collision.

### Map geometry

The water grid uses the same 128-cell row width already seen elsewhere:

- `-128`: up
- `-1`: left
- `+1`: right
- `+127`: down-left
- `+128`: down
- `+129`: down-right

The grid is 100 rows high, inferred exactly from the 12,800-byte clear and the initialization of `$55F34` to 99.

### Relationship to the visible game flood

The interpretation is also consistent with the documented game mechanic: the caverns progressively fill with water, and objects such as the plunger pause the flood while droplets accelerate it. The code here is the first engine subsystem we have found that explicitly maintains a per-cell water state and propagates it through terrain.

## New symbols

| Address | Proposed name | Confidence |
|---|---|---|
| `$17F16` | `water_state[128*100]` | high |
| `$55F34` | `water_scan_row` | high |
| `$F630` | `FindNextWaterCell` | high |
| `$F698` | `AdvanceWaterCellState` | high |
| `$F842` | `CanWaterUseUp` | high |
| `$F87E` | `CanWaterUseLeft` | high |
| `$F8C6` | `CanWaterUseDownLeft` | high |
| `$F90C` | `CanWaterUseRight` | high |
| `$F952` | `CanWaterUseDownRight` | high |
| `$F99A` | `CanWaterUseDown` | high |
| tile attr `0x01` | water-blocking | high |
| tile attr `0x02` | movement-solid | high |

## Next target

The best next milestone is to finish the **39-state `$F698` dispatcher** and trace the flood timing globals around `$F53A/$F5CE/$F9E2`. That should reveal how quickly the flood advances, how pause/speed-up pickups alter it, and how water state maps to rendered water tiles.
