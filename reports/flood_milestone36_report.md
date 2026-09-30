# Flood reconstruction - Milestone 36

## Sprite-bank correction

Milestone 35 contained a renderer identification error. Global sprite IDs `$D2-$D9` are Aunt Matilda, not Quiffy.

Re-checking the original render path together with the recovered artwork gives the corrected pair:

- Quiffy: global sprite bank `$C8-$CF`
- Aunt Matilda: global sprite bank `$D2-$D9`

Both banks use the same compact 0-7 frame selector, which explains the history implementation cleanly: the game records Quiffy's small animation-frame selector in the history ring, then Aunt Matilda replays that selector against her own ghost-art bank.

Conceptually:

```c
quiffy_sprite  = 0xC8 + frame;
matilda_sprite = 0xD2 + frame;
```

where `frame` is the current basic Quiffy animation selector in the range 0-7.

## SDL host correction

The host renderer has been fixed so `flood_player_sprite_id()` returns `$C8 + frame`; the Matilda history renderer remains `$D2 + frame`.

No collision, level, water, or Matilda timing code changed in this milestone.

## Superseded statements

Any previous milestone statement describing `$D2-$D9` as Quiffy artwork should be treated as superseded by this report. Milestone 32's use of `$D2` in the delayed-history renderer was correct; the mistake was interpreting those ghost frames as the live player in Milestones 16/35.
