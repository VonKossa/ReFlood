# ReFlood — Milestone 127

## Original corner movement restored as a complete state machine

Milestone 126's synthetic early snap is removed. A live Level 13 trace of the
original Amiga executable shows that Quiffy does **not** jump directly from
`(56,728)` to `(60,712)`. With Up held for 18 gameplay updates and Right added
without releasing Up, the original follows this path:

| Update | Position | Velocity | Contact | Cache |
|---:|---:|---:|---:|---:|
| 16 | `(56,736)` | `(0,-4)` | `$0E` | off |
| 17 | `(56,732)` | `(0,-4)` | `$0E` | off |
| 18 | `(56,728)` | `(0,-4)` | `$0E` | off |
| 19 | `(56,724)` | `(0,-4)` | `$0C` | wall `(1,-1)` |
| 20 | `(56,720)` | `(0,-4)` | `$0C` | wall `(1,-1)` |
| 21 | `(56,716)` | `(0,-4)` | `$0C` | wall `(1,-1)` |
| 22 | `(56,712)` | `(0,-4)` | `$0C` | wall `(1,-1)` |
| 23 | `(60,712)` | `(4,0)` | `$08` | wall `(1,0)` |
| 24 | `(64,696)` | `(4,-16)` | `$18` | off |

Milestone 125 already matched this particular visible path. The remaining
problem was that its adapted cache logic was not the complete original state
machine, while milestone 123's more exact cache restoration omitted a separate
release instruction and could consequently lock movement.

## Instruction-backed corrections

Three discrepancies were confirmed against the running 68000 code and fixed
together:

1. `$DAEC-$DBE6`: the captured direction pair is again retained for at most
   ten updates and copied into the live direction words before the new contact
   is resolved. The host-only refresh and sign-aware release rules from
   milestones 124-125 are removed.
2. `$DEA4-$DEAC`: after collision resolution, an update whose final X and Y
   movement are both zero clears the attachment mode. This companion rule was
   missing from milestone 123 and is what prevents a blocked cached direction
   from surviving indefinitely.
3. `$DF20`: the side-contact sign tests are corrected. East contact suppresses
   the extra vertical scaling for positive X input; west contact suppresses it
   for negative X input.

The collision extent and inclusive edge calculations in `$F0E0`, the contact
probe in `$F22C`, and the cardinal resolver in `$D99E-$DAEA` were also checked
and already matched the original.

## Verification

- strict C99 core build with all warnings treated as errors: pass;
- complete gameplay, presentation, audio, and data regression suite: pass;
- SDL input/settings regression suite using the SDL stub: pass;
- exact nine-update Level 13 original trace, including position, velocity,
  contact mask, contact count, cached direction, and cache age: pass;
- blocked floor and ceiling updates clear the attachment mode: pass;
- 300-update headless Level 13 smoke test: pass.

The package remains source-only, as in earlier milestones; no executable is
included.
