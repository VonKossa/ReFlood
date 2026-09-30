# `$F0E0` call-site evidence

The binary contains 25 direct `JSR $F0E0` calls. Several gameplay handlers make the argument semantics unambiguous.

## `$1366E`

Stack construction immediately before the call is equivalent to:

```c
F0E0(x, y + 4, +8, 0, 32, 8);
```

The caller then tests bit 1 of `$17E60`, the horizontal-motion result. This is a narrow probe eight pixels to the right of the object.

## `$138D2`

Equivalent shape:

```c
F0E0(x + 8, y + 12, dx, dy, 1, 8);
```

The caller tests bit 1 of `$17E62` and clears its vertical delta when blocked.

## `$139BC`

Registers are loaded directly from the object record:

```text
D1 = object + $00 = x
D2 = object + $02 = y
D3 = object + $04
D4 = object + $06
```

and the call is:

```c
F0E0(x, y, D3, D4, 32, 32);
```

After the collision results are examined, the routine performs:

```c
x += D3;
y += D4;
object->x      = x;
object->y      = y;
object->slot04 = D3;
object->slot06 = D4;
```

This proves that `+04/+06` are X/Y motion components for this handler family, even though other object handlers reuse those same record slots for different purposes.

## `$13A96`

Equivalent call:

```c
F0E0(x + 8, y + 16, dx, dy, 16, 16);
```

Both `$17E62` and `$17E64` are consulted. This is consistent with a compact collision box where the separate diagonal-corner result matters.

## `$13CEA`

Equivalent call:

```c
F0E0(x + 4, y, dx, +1, 24, 32);
```

This is another strong confirmation of `(x,y,dx,dy,width,height)` ordering.

## Collision bit 1

Many callers test bit 1 (`BTST #1`) of `$17E60/$17E62` and cancel or reverse motion when set. It is therefore high-confidence that bit 1 means a solid/blocking tile class. Other tile-attribute bits are intentionally left unnamed until their behavior is traced independently.
