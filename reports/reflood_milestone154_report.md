# ReFlood Milestone 154 Report

## Copyright-safe release data layout

Milestone 154 removes all extracted Flood game assets from the release. The
`reflood_milestone154/data/` directory remains present but is empty.

The extraction tool and its verification manifest remain in `tools/`. Users
must create the data tree from their own supported Flood disk image before
playing or running the gameplay tests.

## Extraction workflow

`tools/extract_flood_data.py` now supports the release layout directly:

```sh
python3 tools/extract_flood_data.py /path/to/Flood.adf
```

The optional output argument still defaults to `./data`. The destination may
be missing or may already exist as an empty directory. A non-empty directory,
regular file, or symbolic link is rejected so existing data cannot be
overwritten accidentally.

Extraction still occurs in a temporary sibling directory. All 207 files are
verified against `tools/flood_data_manifest.json` before the verified tree
replaces the empty placeholder.

## Documentation

The README now:

- states explicitly that releases contain no copyrighted Flood game data;
- explains IPF-to-ADF conversion and the default extraction command;
- documents extraction into the source tree or an already-built playable
  directory;
- explains the empty-directory safety rule;
- notes that gameplay tests require extracted data;
- improves missing-data and non-empty-output troubleshooting.

## Verification

- The supplied supported ADF was extracted into an existing empty `data/`
  directory using the default output path.
- All 207 output files passed size and SHA-256 manifest verification.
- A second extraction into the populated directory was rejected safely.
- The release source `data/` directory remains empty.
- Strict C99 core suite: pass using the independently extracted data.
- SDL input/settings suite: pass.
- UndefinedBehaviorSanitizer core suite: pass.
- Strict headless build and all 42 level smoke runs: pass.

The package is source-only and contains no executable, object file, build
directory, or extracted game-data file.
