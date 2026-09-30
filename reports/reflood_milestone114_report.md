# ReFlood — Milestone 114

## Persistent high scores

The high-score table now persists across launches in `./config/hiscore`.
ReFlood loads this table before ranking the score from the completed game, so
each new result competes against the previously saved top five.

The file is created only after a qualifying name entry has been completed.
Closing the game during name entry does not store a partial name. Writes use a
temporary sibling followed by replacement, and the existing `./config`
directory helper creates the directory when needed.

## Validation and fallback

The saved format has a versioned header followed by five score/name rows.
Loading validates:

- the format version;
- all five complete rows;
- unsigned 32-bit score values in descending order;
- exactly eleven displayable original high-score glyphs per name.

A missing or invalid file leaves the original Flood high-score table intact as
the fallback.

## Verification

- strict C99 core compilation and tests: pass;
- strict C99 SDL input/settings/high-score tests: pass;
- exact `./config/hiscore` creation: pass;
- saved score and 11-character name round trip: pass;
- restored-table ranking of a newly completed score: pass;
- original high-score rendering and editor hashes after insertion refactor:
  pass.
