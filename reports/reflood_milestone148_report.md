# ReFlood Milestone 148 Report

## Scope

Milestone 148 replaces the accumulated development README with a concise
end-user and builder guide. No gameplay, presentation, extraction, or runtime
code was changed.

## README changes

- Removed the license section and all license commentary.
- Removed the complete milestone history and implementation diary.
- Removed low-level reconstruction notes, addresses, evidence summaries, and
  status claims that do not belong in installation instructions.
- Reorganized the remaining information into requirements, data preparation,
  build, run, settings, controls, saved files, tests, and troubleshooting.
- Clarified the required working directory and the `build/reflood` output
  layout.
- Kept the IPF-to-ADF and ADF-to-data workflow, including Windows commands and
  extractor behavior.
- Consolidated keyboard, gamepad, text-entry, and high-score controls.

The README was reduced from 1,473 lines and about 80 KB to 210 lines and about
5.6 KB.

## Verification

- Confirmed that the README contains no occurrence of `license` or
  `milestone`.
- Confirmed that the extractor help command succeeds.
- Re-ran the already-built core and SDL input test executables from the
  unchanged source baseline; both passed.
- A clean rebuild was not possible in the packaging environment because CMake
  is not installed there. No build files or source files changed in this
  milestone.
