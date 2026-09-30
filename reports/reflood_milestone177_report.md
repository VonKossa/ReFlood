# ReFlood Milestone 177: editor save dialog

Ctrl+S now opens an in-editor save dialog. It starts in `custom/levels` (or the `--output` directory's `levels` subfolder) and creates that default folder when needed. The user can browse to another existing folder, edit the base filename, and save with Enter or the Save button. Existing level files require an overwrite confirmation. The dialog remembers the last saved folder and name for the next save.

The editor writes the four matching binary level files using the selected base name: `_header.bin`, `_tilemap.bin`, `_trigger_payload.bin`, and `_triggers.bin`. Ctrl+L now lists and loads these named headers along with numbered original headers. Names use 1–63 ASCII letters, digits, underscores, or hyphens; the editor rejects path separators. The original numbered save and import APIs remain available.

## Verification

- Compiled the editor and its level and UI tests with `-Wall -Wextra -Wpedantic` against SDL2.
- Level round trips passed, including all 42 extracted original levels, named save/load, and invalid filename rejection.
- SDL dummy-renderer interaction tests passed for folder browsing, filename entry, save, overwrite confirmation, reopening through Ctrl+L, and prior editor shortcuts.
- Inspected a 1920×1080 SDL dummy-rendered dialog crop for the title, path, filename field, and folder list. A visible desktop was unavailable, so the dialog still needs confirmation on the user's display. CMake was unavailable in this workspace.

The ZIP includes no extracted copyrighted data. Its empty `data/` directory has mode 755, and timestamps are normalized to 2025-01-01.
