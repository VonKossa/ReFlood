# Flood reverse engineering — Milestone 8

## Result

The `$13A4C` gameplay cluster is identified with **high confidence as the Psycho Teddy**.

The decisive behavioral signature is `$13BA8`: the creature samples the tile underneath/inside itself, recognizes tile IDs `$94,$24,$25,$26,$27`, clears the tile, decrements `$17E72`, and requests a map refresh. `$17E72` therefore behaves as the count of remaining collectible/trash objects. The original manual says Psycho Teddies eat objects, reducing the amount of trash Quiffy must collect.

## Main handler `$13A4C`

High-confidence observations:

- common object fields `x,y,dx,dy` at `+0,+2,+4,+6`;
- vertical acceleration is `+2` per update, clamped to `+16`;
- animation phase at `+9` is advanced/modulated with global `$17E86` and masked to four frames;
- collision query uses a `16x16` body at approximately `(x+8,y+16)`;
- one collision path launches with `dy=-16`, matching the Teddy's leaping behavior;
- a `24x24` overlap with Quiffy writes `-1` to `$E1D0`, i.e. lethal contact;
- while `$17E72 > 0`, it invokes `$13BA8` to consume qualifying map objects;
- it uses `$F50A` as an additional support/terrain probe before movement/rendering.

## Tile-eating helper `$13BA8`

The map location is:

`index = ((x+16)>>4) + (((y+16)>>4)<<7)`

This independently confirms the already-recovered 16-pixel tile size and 128-byte map row stride.

Recognized edible tile IDs:

- `$94`
- `$24`
- `$25`
- `$26`
- `$27`

On a match:

1. `$17E72--`
2. map cell becomes zero
3. `$165DC(16)` is called to refresh/update the changed map area

## New semantic globals

- `$17E72` — high-confidence trash/collectible count remaining
- `$17E86` — global animation tick/phase contribution
- `$E1D0` — Quiffy Life Force (previous milestone; `-1` here means lethal contact)

## Confidence discipline

`Psycho Teddy` is inferred from behavior rather than a symbol in the binary. Confidence is high because the tile-eating side effect is unusually specific and matches the published manual description. The exact semantic contract of `$F50A` and the final sprite-frame arithmetic are not yet fully resolved, so the C reconstruction leaves those explicitly provisional.

## Next target

`$F50A` is now the best next anchor. It appears repeatedly in creature movement code and in the Teddy handler as a terrain/support probe. Fully decoding it should make the movement branch at `$13B50` exact and likely clarify additional enemy handlers around `$13C0A+`.
