# ReFlood

ReFlood is a source reconstruction of the Amiga game *Flood*. It recreates the
original game on modern systems using SDL2.

ReFlood releases do not contain the copyrighted Flood game data. The included
`data` directory is intentionally empty. To play, extract the required data
from your own supported Flood disk image.

## Requirements

- CMake 3.16 or newer
- A C99 compiler
- SDL2 development files
- Python 3 for data extraction
- A legally obtained Flood IPF disk image
- [`disk-analyse`](https://github.com/keirf/Disk-Utilities) with SPS/CAPS IPF
  support

Install the build dependencies on a common platform:

```sh
# Debian / Ubuntu
sudo apt install build-essential cmake libsdl2-dev python3

# Fedora
sudo dnf install gcc cmake SDL2-devel python3

# macOS with Homebrew
brew install cmake sdl2 python
```

On Windows, install CMake, Python 3, a C compiler, and SDL2. SDL2 can be
installed with vcpkg; configure CMake with the vcpkg toolchain file.

## Prepare the game data

Convert the original IPF image to ADF:

```sh
disk-analyse /path/to/Flood.ipf Flood.adf
```

Then run the extraction tool from the ReFlood source directory. The destination
defaults to `./data`:

```sh
python3 tools/extract_flood_data.py /path/to/Flood.adf
```

On Windows:

```bat
py -3 tools\extract_flood_data.py C:\path\to\Flood.adf
```

The extractor accepts only the supported disk revision. It reconstructs and
verifies every required file against `tools/flood_data_manifest.json`. It does
not require a Kickstart ROM or any third-party Python modules.

The destination may be absent or may be the empty `data` directory supplied in
the release. For safety, the extractor refuses to overwrite a non-empty
directory. An optional second argument selects another destination:

```sh
python3 tools/extract_flood_data.py /path/to/Flood.adf data-test
```

ReFlood always loads game assets from `./data` relative to its working
directory.

## Build

From the ReFlood source directory:

```sh
cmake -S . -B build
cmake --build build
```

The playable directory is `build/reflood`:

```text
build/reflood/
├── reflood        # reflood.exe on Windows
├── reflood-editor # reflood-editor.exe on Windows, when SDL2 is installed
├── data/
└── tools/
```

CMake copies the extracted source `data` tree into this playable directory. If
you built ReFlood before extracting the game data, extract directly from the
playable directory instead:

```sh
cd build/reflood
python3 tools/extract_flood_data.py /path/to/Flood.adf
```

## Run

ReFlood uses paths relative to its working directory. Start it from the
playable directory:

```sh
cd build/reflood
./reflood
```

On Windows:

```bat
cd build\reflood
reflood.exe
```

An optional command-line argument selects the initial level:

```sh
./reflood 12
```

Valid level numbers are 1 through 42. Normal play begins at level 1 and uses
the in-game level-code screen.

## ReFlood-Editor

`reflood-editor` is built beside `reflood`. Run it from the playable directory
with extracted game data. It starts with a blank level 1; import an original
level or create another blank one with:

```sh
./reflood-editor --input data --level 1
./reflood-editor --new --level 2
```

The editor needs the extracted block graphics and level font even for blank
maps. `--data DIR` selects those assets, `--input DIR` selects imported levels,
and `--output DIR` changes the output root (default: `custom/levels`).

Left drag paints the selected tile; right click picks a tile from the map.
Scroll or use arrow keys to pan, and use `+`/`-` or Ctrl+mouse wheel to zoom.
`B` changes the palette bank, and `T` opens descriptions for tile IDs 000–255.
Tile 22 marks the player start; tile 23 sets camera bounds. The sidebar lists
the main shortcuts: Ctrl+G toggles the grid, Ctrl+Z undoes a stroke, Ctrl+F
toggles fullscreen, and Ctrl+Q asks before quitting.
While the editor window has focus, SDL2 2.0.16 or newer grabs the keyboard so
desktop shortcuts such as an XFCE Ctrl+S binding do not take precedence. The
grab is released when focus moves away or the editor closes. Some system
shortcuts cannot be captured; fullscreen Alt+Tab remains available by default.

Ctrl+S opens a save dialog in `custom/levels` by default. Enter a base filename
or select an existing header; overwrites require confirmation. Ctrl+L browses
folders and `*_header.bin` files to load a level. Use Enter or double click to
open, Backspace to go up a folder, `C` for `custom/levels`, and `D` for the
extracted `data/levels`. Loading warns about unsaved edits. Trigger creation
and editing are not yet available.

## Custom maps

Saving `my_cave` writes `my_cave_header.bin`, `my_cave_tilemap.bin`,
`my_cave_trigger_payload.bin`, and `my_cave_triggers.bin` together. In ReFlood,
choose its header with **Choose Custom Map**, then start single-player or
multiplayer. The browser starts in `./custom/levels`, or `./` if that folder is
missing. Delete clears the selection; Esc keeps it.

A standalone name plays once. For a sequence, name the files `level_01_*`,
`level_02_*`, and so on through `level_99_*` in the same folder. Selecting 03
starts at 03; each exit loads the next numbered header. A missing number ends
the sequence. **CONGRATULATIONS** appears for three seconds after the final
map, on both views in multiplayer, before returning to Settings. Custom maps
use your extracted assets but do not enter the original ending or high scores.

## Play and input

The initial menu lets you choose display scaling, game speed, and Player 1 and
Player 2 inputs. Player 1's selection also controls single-player mode.

### Experimental local multiplayer

Assign two distinct inputs (Arrows / Right Ctrl, WASD / Left Ctrl, or connected
gamepads) and choose **Start Multiplayer (Experimental)**. Players share the
cavern, trash count, and exit, but have separate scores, health, lives,
weapons, cameras, and Matilda ghosts. The first player to reach the exit after
all trash is collected advances both. Weapons and either ghost can hurt either
player; each player can collect a weapon pickup independently.

The 16:9 split view has a black divider and each player's in-game HUD. A player
who loses all lives sees **GAME OVER** while the other can continue. **P**
pauses until either player presses and releases Fire; **R** restarts and costs
both surviving players a life; Escape starts both death sequences. The
original campaign uses the single-window intro, level selector, ending, and
shared high-score screen. A selected custom map starts directly in the split
view and follows the numbered sequence described above.

### Keyboard

| Key | Action |
| --- | --- |
| Arrow keys or WASD | Move, according to the selected player input |
| Right or Left Ctrl | Fire or confirm, according to the selected player input |
| P | Pause; press Fire to continue |
| R | Restart the level and lose one life |
| F | Toggle fullscreen for the current session |
| Escape | End the current game |

`F` does not change the saved display setting; on text-entry screens it enters
text instead of toggling fullscreen.

### Gamepad

| Control | Action |
| --- | --- |
| D-pad or left stick | Move |
| A | Fire or confirm |
| Back | End a single-player run |

Connected SDL-compatible controllers are named in the input choices and can
be attached or removed while ReFlood runs.

### High-score entry

If a score qualifies, use Up and Down to choose a character, Left and Right to
move between positions, and Fire to confirm. Select `:` to finish early.

## Saved files

ReFlood creates `./config` when needed:

```text
config/
├── configuration  # display, scaler, speed, and player assignments
└── hiscore         # persistent high-score table
```

If a saved high-score file is missing or invalid, the original default table
is used. A high-score entry is saved only after name entry is completed.

## Tests and headless build

The automated gameplay tests require the extracted `./data` tree. Run them
after preparing the game data and completing a normal build:

```sh
ctest --test-dir build --output-on-failure
```

SDL2 can be disabled for core development or automated testing:

```sh
cmake -S . -B build-headless -DFLOOD_WITH_SDL2=OFF
cmake --build build-headless
ctest --test-dir build-headless --output-on-failure
```

The headless executable performs a short deterministic simulation; it is not a
playable text-mode version of the game.

## Troubleshooting

- **SDL2 was not found:** install the SDL2 development package, remove the
  CMake build directory, and configure again.
- **A level or asset cannot be loaded:** run ReFlood from its playable
  directory and confirm that `./data` contains the extracted files. An empty
  directory is only the release placeholder.
- **The extractor rejects the ADF:** use an unmodified ADF converted from the
  supported original Flood IPF image.
- **The extractor says the output is not empty:** move or remove that data
  directory before extracting. Existing files are never overwritten.
- **No gamepad input:** connect an SDL-compatible controller and select
  it for Player 1 or Player 2 in the initial menu.
