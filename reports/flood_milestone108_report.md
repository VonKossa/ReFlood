# Flood reconstruction — Milestone 108

## Original-data installation documentation

The README now documents the intended data-free public distribution model.
Players will supply their own legally obtained Flood IPF or supported 880 KiB
ADF image, and a forthcoming `flood-data-extract` utility will construct the
external `./data` directory beside the executable.

The instructions specify:

- the final directory layout and command-line invocation;
- separate SPS/CAPS decoder installation for IPF input;
- direct ADF input without an IPF decoder;
- no requirement for a Kickstart ROM or emulator boot;
- disk-revision detection and per-file size/SHA-256 verification;
- failure without leaving a seemingly valid mixed or partial installation.

The README explicitly marks the extractor as not yet implemented. This avoids
presenting the documented commands as available in the current milestone. The
development archive continues to include recovered data for testing, but the
README warns that it must be removed from a public release until the extractor
and verification manifest are complete.

## Verification

- Markdown structure and fenced examples inspected: pass
- README references to `flood-data-extract`: present and status-qualified
- source and runtime behavior: unchanged from Milestone 107
