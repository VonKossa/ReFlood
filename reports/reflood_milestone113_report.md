# ReFlood — Milestone 113

## Explicit settings save

The startup menu now separates editing, saving, and starting:

- `DISPLAY: WINDOWED` / `DISPLAY: FULLSCREEN` changes only the pending value;
- `SAVE SETTINGS` applies the pending mode and writes it to
  `./config/configuration`;
- `START GAME` continues without applying or saving an outstanding change.

The menu now has three selectable rows. Up/Down wraps through them, and the
existing edge latches prevent held directions or confirmation buttons from
repeating.

## Temporary in-game fullscreen toggle

The in-game `F` shortcut still toggles borderless desktop fullscreen, but it no
longer changes `./config/configuration`. The next launch therefore uses the
last choice explicitly committed with `SAVE SETTINGS`.

## Verification

- strict C99 core compilation and tests: pass;
- strict C99 SDL input-stub compilation and tests: pass;
- pending display changes do not invoke SDL mode switching: pass;
- Save and Start produce distinct menu actions: pass;
- three-row navigation and wraparound: pass;
- in-game `F` leaves the configuration file absent/unchanged: pass;
- configuration parsing, persistence, font, and monochrome rendering tests:
  pass.
