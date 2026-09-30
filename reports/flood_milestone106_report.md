# Flood reconstruction — Milestone 106

## Escape abort during immortality

Escape already set the lives counter to zero and exhausted Life Force, but it
returned before entering Quiffy's death state. If Cocktail/immortality
protection was active, the following gameplay update decremented its timer and
restored Life Force to full. Quiffy therefore remained alive until protection
expired and Escape was pressed again.

Escape is now handled as an explicit final-life abort. On its first update it:

- sets the lives counter to zero;
- exhausts Life Force;
- clears the active invulnerability timer and blink state;
- enters death mode 1 immediately through the shared death routine.

The existing airborne/wall fall, floor landing, and `$78-$7F` death-cross
animation remain intact. When that animation completes, the zero lives count
ends gameplay and enters the post-score title/high-score loop. Normal damage
and fatal terrain still respect active immortality; only Escape bypasses it.

## Regression coverage

- ordinary Escape starts death mode on its first update;
- Escape with 50 immortality ticks remaining clears protection immediately;
- Life Force cannot be restored on the following update;
- both protected and unprotected aborts finish the cross animation with zero
  lives and stop gameplay.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- headless load/run smoke test for all 42 levels: pass
