# ReFlood Milestone 181: tile-reference audit

The T: Tiles popup has been audited against the reconstructed runtime, the three extracted BLOCK attribute tables, and the 42 original level maps. All IDs 000–255 remain in numeric order. Compared with milestone 180, 224 descriptions per bank have changed.

- Markers 001–025 now identify their confirmed spawned creatures, mechanisms, water source, player start, or camera setup. Markers that only clear themselves say so. Tile 013 is identified as a direct runtime explosion state.
- Animation bases 029, 032, and 071 describe their frame cycles; 030, 031, and 072 explain that placing a frame alone leaves it static. The 071/072 entries indicate fatal contact only in the banks whose attributes carry it.
- Transport IDs 156–175 name their exact partner tile. Mechanism graphics 200–205 identify their role without implying that placing the graphic itself spawns a mechanism.
- Pickups and special tiles 219–238 distinguish the bounce effect, Cocktail, Orange Can, active Mine graphic, persistent score/zap tile, and the six weapon pickups.
- Ordinary terrain describes known physics from the currently selected bank: solidity, water blocking, slope and slope correction, fatal contact, and the straight-down support check. The remaining unclassified attribute bit 3 is shown in hex rather than given an unsupported meaning.

The popup's header shows the visible ID range; a footer clarifies that marker pictures are raw tile symbols, not runtime sprites. Some artwork still lacks a verified individual name. Those entries deliberately describe properties instead of guessing from appearance. The distributed package includes no extracted game graphics or attribute bytes.

## Verification

- Compiled the SDL2 editor and UI and level tests with `-Wall -Wextra -Wpedantic`; both test programs passed.
- Checked all 256 entries for nonempty descriptions, reciprocal transport pairs, and the Bank A/B difference for tile 071's fatal attribute.
- Inspected a 1920×1080 SDL dummy render of the Bank B reference showing IDs 065–085, including lethal animated 071/072. Text and graphics fit within the popup. A visible desktop was unavailable.

The ZIP's empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
