# Flood Milestone 66: exact water propagation

## Result

Milestone 66 translates the flood engine at `$F53A-$F9E0`, its frame
scheduler at `$AF2E-$AF74`, and its trigger-driven reconstruction at
`$F5CE-$F62E`. Secondary-map state is no longer limited to marker-21's initial
value 4: it now falls, spreads, fills, rises, pauses, accelerates, and rebuilds
through the original control flow.

## Persistent state

The original engine has two private cursors in addition to the 128x100 map:

| Address | Host field | Reset value | Purpose |
| --- | --- | ---: | --- |
| `$55F34` | `water_scan_row` | 99 | bottom-up row selector |
| `$F9E2` | `water_age` | 1 | successful state-update count/replay budget |

`$F55A` clears all 12,800 secondary-map bytes, restores those values, and does
not alter the source coordinates, pause, speed, or active flag.

## Selector at `$F630`

The selector examines one 128-cell row from left to right. It returns the
first cell with the smallest positive state below 40. If no eligible cell is
present, it decrements the persistent row and repeats until row zero. Falling
water increments the cursor when it creates a cell in the row below, allowing
the selector to follow that new leading edge.

This is important: the engine is ordered and incremental, not a simultaneous
cellular-automaton pass. Equal states are resolved left-to-right.

## State dispatcher at `$F698`

The 41-entry PC-relative jump table produces these state families:

| Input state | Operation |
| --- | --- |
| 1, 2 | create falling state 5 or 6 below; if blocked, become 8 |
| 4 | become 8 |
| 5, 6 | continue the same falling state below; if blocked, become 9 or 10 |
| 8-10 | add 4 |
| 12-14 | try left, then right; create edge 1/2 over a drop or state 12 over support; add 4 when enclosed |
| 16-18, 20-22, 24-26, 28-30, 32-34 | add 4 |
| 36-38 | become 40 and raise/update the cell above |
| 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 40 | unchanged |

Side and downward destinations require terrain attribute bit 0 to be clear and
the destination water byte to be zero. The upward operation checks only the
terrain bit, then writes 4 when the prior state is below 3 or adds 4 otherwise.
Each dispatched cell increments `water_age`, even when its jump-table entry
does not change the byte. A failed selection does not increment it.

## Frame scheduler and pickups

When water is active, `$AF2E-$AF58` either decrements `$17E7C` or calls the
single-cell routine `$17E7E²` times. If speed is above 1, `$AF60-$AF74` then
subtracts the current alternating display-buffer index. The host reproduces
that order exactly.

The main pickup handler supplies the two shipped controls:

| Tile | Original write | Effect |
| ---: | --- | --- |
| `$E4` | `$17E7E = 6` | accelerated flood; requests sound 10 |
| `$E5` | `$17E7C = 100` | pauses propagation for 100 updates |

The decoded maps contain 90 `$E4` cells and 20 `$E5` cells.

## Trigger rebuild at `$F5CE`

After a mode-0 trigger swap, `$12DF6-$12E0A` invokes the rebuild whenever
water is active. The routine saves `water_age`, clears the complete secondary
map, re-seeds state 4 at the saved marker-21 coordinates, replays up to that
many selected cells, and restores the saved age.

The source-X test is literal. A source at X=0 fails it and is not re-seeded;
the host retains this original edge-case behavior. With multiple marker-21
cells, initial loading seeds each one, while the saved coordinates—and thus a
later rebuild—refer to the last marker processed.

## Verification

- strict C99 with `-Wall -Wextra -Wpedantic -Werror`: pass;
- full core regression: `core ok`;
- UBSan with abort on first error: pass;
- all 42 levels complete the 300-tick headless run;
- focused tests cover falling states, blocked transitions, left priority,
  supported and unsupported lateral spread, all fill families, upward rise,
  pause, squared speed budget, alternating-buffer decay, replay, and X=0;
- Level 1 after 1,000 steps is locked to scan row 14 and 149 nonzero cells,
  including 100 cells in terminal state 40.
