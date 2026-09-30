# Flood Milestone 91: flooded airborne-death correction

## Result

A dead Quiffy now falls automatically from a wall or ceiling until reaching
south support, including when the cavern is deeply flooded. The grounded
`$78-$7F` death-cross sequence then begins without requiring movement input.

## Root cause

Milestone 88 implemented the original `$DBE8-$DC28` contact-word clear, but its
regression stopped after only two dry falling updates. It never tested the
complete wall-to-floor transition or a flooded cavern.

After clearing the contact word, the host reused its generic no-contact motion
branch. That branch applies water buoyancy and has a held-down override. In
deep water the buoyancy could cancel or reverse death-mode gravity, pinning
Quiffy to a wall or ceiling. Pressing down selected the override and made him
descend, matching the reported symptom.

The host also retained `attachment_mode`, `attachment_ticks`, and the cached
surface directions even though those fields are derived from the contact word
that the original code completely clears.

## Correction

While `death_mode == 1` and no south-support contact exists, the host now:

- clears the complete contact state and its derived attachment cache;
- advances through the dry gravity calculation regardless of water depth;
- suppresses the ordinary held-down water-motion override.

Water state and oxygen accounting remain active. Horizontal collision,
floor-contact detection, the eight-pixel landing alignment, sound `$38`, and
the mode-2 cross animation are unchanged.

## Regression coverage

- full-height east wall to floor while up/right remains held: pass;
- ceiling to floor while up remains held: pass;
- fully flooded full-height wall to floor without held-down input: pass;
- grounded death lifecycle and `$78-$7F` sprite cadence: pass;
- strict C99 core regression suite: pass;
- core suite under UndefinedBehaviorSanitizer: pass.
