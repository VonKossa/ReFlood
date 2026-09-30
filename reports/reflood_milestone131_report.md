# ReFlood — Milestone 131

## GPLv3 license added

The project root now contains `LICENSE` with the complete GNU General Public
License, version 3 text. The README identifies ReFlood's reconstruction source
code and original project tooling as GPLv3-licensed.

The README also makes the licensing boundary explicit: extracted graphics,
levels, music, sound effects, and other original Flood game data are not
covered by ReFlood's GPL license. They remain copyrighted original-game
content and are not intended for public distribution with ReFlood.

## Canonical package structure

Milestone 131 is based directly on the user-reorganized milestone 130 archive.
Its structure is retained as the template for future releases:

- `docs/` contains reference documentation;
- `pictures/` contains generated previews and analysis images;
- `reports/` contains all historical and current milestone reports;
- `data/`, `include/`, `src/`, `tests/`, and `tools/` retain their existing
  project roles;
- only `CMakeLists.txt`, `README.md`, and `LICENSE` are regular files in the
  project root.

## Verification

- official GPLv3 text present at project-root `LICENSE`: pass;
- GPLv3 license heading and version verified: pass;
- reorganized root directory and report/image locations preserved: pass;
- strict C99 core build with warnings treated as errors: pass;
- complete core regression suite: pass;
- SDL host regression suite: pass.

The package remains source-only; no executable is included.
