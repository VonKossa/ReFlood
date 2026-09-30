# Flood reverse engineering -- Milestone 11

## Result

The `$F698` flood-cell state machine is now structurally lifted, and the Plunger/Droplet timing controls have been located.

## The 40-entry dispatcher

`$F698` reads the byte at `$17F16[index]`, doubles it, indexes a 40-word PC-relative jump table at `$F6BA`, and executes the selected state handler. `$F630` schedules only values `1..39`; values >= 40 are ignored.

The handlers are not 39 independent behaviors. They collapse into a small number of patterns:

- downward propagation, seeding the cell below with state 5 or 6;
- four-step phase advancement (`state += 4`);
- lateral spreading from states 11-13;
- finalization at states 35-37, where the current cell becomes state 40;
- terminal/no-op scheduler phases.

This strongly suggests that the numeric values combine water geometry with animation/propagation phase.

### Lateral spreading

For states 11-13, the engine checks left and right independently. When a side is open it also checks the corresponding down-diagonal cell:

- left + down-left open -> left child state 1;
- left open but down-left blocked -> left child state 12;
- right + down-right open -> right child state 2;
- right open but down-right blocked -> right child state 12.

That is a compact rule for water falling around edges versus spreading across supported surfaces.

### Final state

States 35-37 set the current cell to `40`. Because `$F630` explicitly rejects values >= 40, state 40 is a settled/terminal state from the scheduler's perspective. Those handlers may also propagate a phase into the cell above.

## `$F53A`: advance one scheduled cell

`$F53A` calls `$F630` to find the next active cell. If an index is returned it invokes `$F698(index)`. This is the primitive "perform one flood simulation step" operation.

## Flood timing: `$17E7C` and `$17E7E`

The update path around `$AF30` makes the control variables clear:

- `$17E7C`: flood pause countdown;
- `$17E7E`: flood speed value.

When pause is non-zero, it is decremented and no `$F53A` steps run. Otherwise the code calculates `speed * speed` and invokes `$F53A` that many times during the update.

Initialization writes `speed = 1` and `pause = 0`.

## Droplet and Plunger

Two independent item-dispatch paths write the same values:

- one pickup writes `6` to `$17E7E` -> flood speed-up;
- another writes `100` to `$17E7C` -> flood pause.

These effects match the documented Droplet (speeds the flood) and Plunger (temporarily stops it) mechanics. The semantic names are therefore high confidence even though the original symbols are lost.

Recovered C helpers are in `src/water_control.c`.

## `$F5CE`: rebuild/fast-forward water simulation

`$F5CE` saves the longword at `$F9E2`, clears/reinitializes the water grid via `$F55A`, seeds one map-derived water cell with state 4, then repeatedly calls `$F630/$F698` before restoring `$F9E2`. This looks like deterministic reconstruction/fast-forward of water state from a stored progress count rather than ordinary frame-by-frame updating.

## New symbols

| Address | Proposed name | Confidence |
|---|---|---|
| `$F53A` | `AdvanceOneWaterStep` | high |
| `$F698` | `AdvanceWaterCellState` | high |
| `$F9E2` | water simulation progress/step counter | high |
| `$17E7C` | `water_pause_ticks` | high |
| `$17E7E` | `water_speed` | high |
| state 40 | settled/unscheduled water cell | high |
| pickup effect `speed=6` | Droplet | high |
| pickup effect `pause=100` | Plunger | high |

## Next target

The next high-value target is the rendering/conversion path that reads `$17F16` at `$131AE-$13296`. That should tell us how these 0..40 internal water states map to visible water tiles/animation frames, and may let us give semantic names to the state ladders instead of numeric labels.
