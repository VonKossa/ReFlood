# ReFlood — Milestone 111

## Release-package cleanup

The release tree no longer contains stale generated compiler and verification
output inherited from earlier milestones:

- nine top-level `build-*` directories;
- all top-level `cc*.o` temporary object files;
- all top-level `cc*.s` temporary assembly listings;
- Python `__pycache__` output.

These files were not referenced by the current CMake configuration, source,
tests, extraction tools, or documentation. Approximately 42 MB of uncompressed
generated output was removed. The cleaned release tree retains the complete
source, tests, data-extraction workflow, milestone reports, preview artefacts,
and the requested development `data/` directory.

## Distribution naming

- archive: `reflood_milestone111_package.zip`
- archive root: `reflood_milestone111/`
- report: `reflood_milestone111_report.md`

## Verification

- forbidden top-level `build-*` entries: none
- forbidden top-level `cc*.o` entries: none
- forbidden top-level `cc*.s` entries: none
- strict C99 core compilation and tests from cleaned tree: pass
- strict C99 SDL input-stub compilation and tests from cleaned tree: pass
- strict C99 headless `reflood` build and level-1 smoke run: pass
- ADF extraction and verification of all 207 data files: pass
