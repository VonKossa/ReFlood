# Flood reconstruction — Milestone 107

## Portable external-data binary

Game data remains external to the executable. The supported distribution
layout is:

```text
flood/
├── flood          # flood.exe on Windows
└── data/
```

The production host now loads assets from `./data`. The former public CMake
definition embedded the source tree's absolute `data` path into every target,
which made a built executable dependent on the build machine. That definition
has been removed from the production library and host.

The two test targets still receive the absolute source-data directory because
CTest normally runs them from the build directory. This test-only definition
does not propagate into the shipped executable.

## Build and installation layout

The CMake host target is named `flood` on disk and is written to the
build directory's `flood/` subdirectory. A post-build rule copies the complete
external data tree beside it. Installation uses the same `flood/flood[.exe]`
and `flood/data/` structure.

The executable intentionally resolves `./data` from its working directory.
It should therefore be launched from inside the containing `flood/` directory,
as it will be when opened normally from that distribution folder.

## Regression coverage

- the production host compiles without a `FLOOD_DATA_DIR` override;
- the resulting binary contains the literal `./data` path;
- it contains no absolute workspace/source-data path;
- a separate staged `flood/` directory contains all 207 external data files;
- the relocated binary loads and smoke-runs every level from 1 through 42.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- relocated `./data` headless smoke test for all 42 levels: pass
