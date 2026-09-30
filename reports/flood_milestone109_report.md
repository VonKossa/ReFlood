# Flood reconstruction — Milestone 109

## Transparent original-data extraction

Milestone 109 replaces the proposed installer with a documented, manual
two-command workflow:

1. `disk-analyse` converts a player-owned original Flood IPF to ADF.
2. `tools/extract_flood_data.py` reconstructs `./data` from that ADF.

The Python script uses only the standard library. It reads the disk's compact
directory at `0x1600`, extracts direct resources, implements the original
literal/absolute-copy packed format, divides the eleven grouped map resources
into all 42 levels, and derives the level trigger lists, password table, and
copy-protection message offsets.

`tools/flood_data_manifest.json` contains only expected paths, sizes, and
SHA-256 digests. Extraction takes place in a temporary sibling directory. The
script accepts only the verified supported ADF revision, verifies all 207
generated files, refuses to overwrite an existing output directory, and
removes the temporary tree on failure.

The CMake build and install layouts now include the script and manifest under
`flood/tools/`. The development `data/` directory remains in this milestone as
requested; no distribution-data removal has been performed.

## Verification

- supported 901,120-byte ADF recognized: pass
- all disk-directory entries parsed: pass
- all packed members consumed without trailing bytes: pass
- generated file count: 207
- generated tree byte-identical to the development data: pass
- no Kickstart ROM or emulator used for extraction: pass
