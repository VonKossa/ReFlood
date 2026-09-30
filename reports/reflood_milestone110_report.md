# ReFlood — Milestone 110

## Project and executable rename

The reconstruction is now named **ReFlood**. The CMake project is `ReFlood`,
the production host target and executable are both named `reflood`, and the
portable build directory is `build/reflood/`.

The SDL window title is now exactly `ReFlood`.

The build and install rules stage the existing external `data/` directory and
the Milestone-109 extraction tools beside `reflood` under the renamed
`reflood/` distribution directory. Internal `Flood*` types and the
`FLOOD_DATA_DIR` definition remain unchanged because they describe the original
game format and are not user-facing product names.

The README heading, build commands, executable examples, working-directory
instructions, and layout diagrams have been updated for ReFlood. Historical
milestone reports retain their original names and wording.

## Distribution naming

- archive: `reflood_milestone110_package.zip`
- archive root: `reflood_milestone110/`
- report: `reflood_milestone110_report.md`

## Verification

- strict C99 core compilation and tests: pass
- strict C99 SDL input-stub compilation and tests: pass
- exact `ReFlood` SDL-title regression assertion: pass
- strict C99 headless `reflood` compilation: pass
- relocated `./data` level-1 smoke run: pass
- legacy `flood_host` target/output references in current build files: none
