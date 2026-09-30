# Flood reverse engineering — Milestone 57 (corrected)

## Result

Sparkling Fungi belongs to Flood's static fatal-terrain system, not the
25-state object dispatcher. The original executable marks lethal terrain with
attribute bit 6 (`$40`), promotes that attribute to Quiffy's contact-mask bit 8
(`$0100`), and enters death mode 1 on contact unless cocktail invulnerability
is active.

The previous Milestone 57 conclusion is withdrawn. Sprite frames `$7D-$7F`
are part of Quiffy's death/respawn cross and are not Sparkling Fungi.

## Correcting the false sprite identification

The player death routine makes the complete sprite range explicit:

- `$CE68-$CE72` clamps the death animation phase to 7, then adds `$28`;
- `$D164-$D17A` draws sprite `$50 + phase`;
- therefore phases `$28-$2F` draw global sprites `$78-$7F`.

The earlier sprite-call audit looked for literal IDs and failed to account for
this computed range. The attached preview now labels all eight frames as the
death/respawn cross.

## Exact fatal-contact path

The recovered 68000 path is:

| Address | Operation |
| ---: | --- |
| `$F22C` | Edge-query builder tests terrain attribute bit 6 (`$40`). |
| `$F270-$F27C` | First horizontal edge promotes it to contact bit 8. |
| `$F29A-$F2A0` | Second horizontal edge promotes it to contact bit 8. |
| `$F30C-$F318` | First vertical edge promotes it to contact bit 8. |
| `$F336-$F33C` | Second vertical edge promotes it to contact bit 8. |
| `$F476-$F47E` | Cocktail timer `$E1CE` clears contact bit 8 while active. |
| `$D87E-$D894` | If Quiffy is alive and bit 8 remains set, set death mode 1. |

This also explains why the earlier Life Force-reference audit missed the
mechanism: fatal terrain does not directly store `-1` to Life Force. It enters
the player's death state instead.

## Shipped fatal tiles

Seventeen tile definitions carry bit 6; thirteen are placed in the 42 shipped
levels, for 951 fatal map cells in total: 885 from BLOCKA, 56 from BLOCKB, and
10 from BLOCKC. The large count is expected because the same bit represents a
general lethal-surface property across several visual themes.

The visually clearest fungus/spike candidate is BLOCKB tile `$47`: it is a
purple crystalline growth and occurs 56 times across levels 16, 20, 21, and
37. BLOCKA `$3D/$3E` and BLOCKC `$47/$48` demonstrate that the same collision
property is shared by other pointed and hot-cavern hazards. The binary proves
the behavior exactly; the specific printed name “Sparkling Fungi” is an art
theme applied to this general terrain mechanism rather than a separate object
state.

## SDL integration

The host already reconstructed the underlying behavior but expressed the
attribute as the magic value `0x40`. Milestone 57 now:

- defines `FLOOD_ATTR_FATAL` as `0x40`;
- uses it in both horizontal and vertical contact aggregation;
- retains contact-mask bit `0x0100` and cocktail protection;
- adds regression coverage for fatal contact and the protected-contact bypass.

The current compact host models a fatal contact as immediate life loss and
respawn. The original's complete death-mode presentation and timing remain a
separate reconstruction boundary.

## Validation

The corrected preview is generated from the recovered four-bitplane sprite and
tile data using the verified live gameplay palette and natural plane order.
The strict C99 test suite, undefined-behavior sanitizer build, headless host,
and all-level smoke run are used to validate this milestone.

## Next boundary

The next high-value target is either the complete Quiffy death-mode lifecycle
that this path enters or the weapon/action subsystem: eight 18-byte records and
the dispatcher at `$CB40`.
