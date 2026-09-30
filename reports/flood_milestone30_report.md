# Flood reverse engineering - Milestone 30

## Scope

This pass tested the hypothesis that **Sparkling Fungi** might be encoded as a special tile-attribute class. It also catalogued all nonzero attribute bytes in the three decoded `BLOCKA/B/C` banks and traced which bits are actually consumed by `RUN_PROG`.

## Result: there is no generic lethal-terrain bit

The generic tile attribute byte has a clean split between two established flags and several geometry/response modifiers:

- `0x01`: blocks water propagation.
- `0x02`: primary solid/blocking flag for actors.
- `0x04..0x80`: special collision geometry / response modifiers used by Quiffy's contact resolution and projectile collision.

No direct path was found from any of these attribute bits to an instant Life Force kill. In particular, searches around every `$17E60/$17E62/$17E64` collision-result consumer and `$17F1A` attribute-table access did not produce a generic lethal-tile branch.

Therefore the earlier plan to identify Sparkling Fungi by finding a hypothetical `LETHAL` bit in the attribute table is rejected.

## High-bit geometry evidence

Bits 6/7 (`0x40/0x80`) are strongly associated with slope/edge-shaped tiles in the decoded graphics and are tested by the `$F22C` contact-mask construction code. This is consistent with Flood's unusual wall/ceiling traversal: the collision metadata has to encode more than binary solid/empty geometry.

Bit 2 is consumed by Quiffy's main collision resolver, and bits 4/5 are consumed in the shuriken collision path. Their original names are not yet recoverable with confidence, so the C header intentionally leaves them as numbered modifiers.

## Attribute-value distribution

Only a small vocabulary of attribute bytes occurs:

- Bank A: `00, 02, 03, 06, 0A, 13, 23, 40, 43, 82, 83`
- Bank B: `00, 02, 03, 06, 08, 13, 23, 40, 83`
- Bank C: `00, 02, 03, 06, 0A, 13, 23, 40, 43, 83`

The accompanying special-tile sheet renders every tile whose attribute contains bits above `0x03`.

## Sparkling Fungi search direction

The strongest remaining model is now:

```text
Sparkling Fungi
    -> static map content or stationary special object
    -> dedicated overlap / tile-ID check
    -> instant player death
```

not:

```text
ordinary terrain collision
    -> lethal attribute bit
```

The next static pass should therefore focus on **map-cell ID consumers outside the generic item dispatcher and `$F0E0`**, especially code that samples the cells around Quiffy's body and routes them through special-object logic.

## State 23

This pass does not change the state-23 conclusion. Marker/trigger 23 is level-bounds setup and is unrelated to runtime behavior state 23. The runtime `23 -> 24 -> 25 -> 26` object still requires either the indirect creator path or a WinUAE write watchpoint to identify conclusively.
