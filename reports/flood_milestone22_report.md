# Flood reverse engineering — Milestone 22

## Summary

This milestone links the common post-hit states back to named creature behavior and narrows the origin of the unusual three-hit destructible.

## Named combat linkage

### Beady Ball

Runtime state **5** dispatches to `$13972`, already identified as the ricocheting Beady Ball. Weapon hits advance it to state **6**, which dispatches to `$10A10` and ends in removal. **Beady Ball does not use the Heart-producing death path.**

### Psycho Teddy

Runtime state **12** dispatches through `$10A92`. When object subtype `+0x0E == 0x38`, it calls `$13A4C`, the Psycho Teddy handler. Weapon hits advance state 12 to **13**, which dispatches to `$1091A`. Therefore **Psycho Teddy dies into a Heart pickup.**

### Bulbous Headed Vong

The same state-12 dispatcher checks subtype `+0x0E == 0x6E` and calls `$13CA0`. That handler periodically invokes `$13C0A`, which writes trash tile IDs (`$94,$24,$25,$26,$27`) into empty map cells and increments `$17E72`, the trash-remaining counter.

This behavior is an unusually specific match for the manual's Bulbous Headed Vong: they throw trash around and increase how much Quiffy must collect. Therefore subtype `0x6E` is identified with high confidence as **Bulbous Headed Vong**.

Because Vong uses runtime state 12, weapon hits likewise advance it to state 13 and the **Heart-producing death path**.

### State 1: likely Lumpy Wanderer

Runtime state **1** dispatches to `$13782`. Its motion code repeatedly resolves local surface contacts and changes its movement/orientation while continuing along walls/ceilings. That is a strong match for the manual's **Lumpy Wanderer**, described as floating around while following walls and ceilings.

This identification remains inferential rather than source-symbol proof. State 1 advances to state 2 on weapon hit, so this creature family also produces a Heart.

## Remaining normal hittable states

- state **8** -> `$10834` -> state 9 on hit -> `$10A10` non-Heart death.
- state **14** -> `$10C46` -> state 15 on hit -> `$10A10` non-Heart death.

State 14 has conspicuous bouncing/leaping vertical behavior, making **Plonkin Donkin** a plausible candidate, but that label is not yet strong enough to lock in. State 8 likewise remains unnamed for now.

## Strong side result: likely Sparkling Fungi

Runtime state **22** dispatches to `$1363A`. It is initialized as a mostly stationary animated object and instantly kills Quiffy on overlap. That closely matches the manual's **Sparkling Fungi**, which do not move and instantly drain all Life Force.

This is marked as a strong candidate rather than final because visual sprite confirmation is still missing.

## The three-hit destructible is dynamic

The level-object initializer at `$E7B0` was mapped across all marker values 0..25. Crucially, level markers **23, 24, and 25 do not instantiate runtime states 23, 24, and 25**. They perform other level setup.

Therefore the special weapon-hittable chain:

```text
23 -> 24 -> 25 -> 26
```

must be created dynamically elsewhere in the game. This rules out the simplest interpretation that it is merely "level object type 23" and gives the next pass a much smaller search space: trace writes/copies into runtime records that can create behavior 23 after level initialization.

## Recovered combat taxonomy

```text
Heart-producing:
  state 1  -> 2  -> $1091A -> Heart
  state 12 -> 13 -> $1091A -> Heart
    - Psycho Teddy (subtype 0x38)
    - Bulbous Headed Vong (subtype 0x6E)
    - additional state-12 variants

Non-Heart explosion/removal:
  state 5  -> 6  -> $10A10  (Beady Ball)
  state 8  -> 9  -> $10A10
  state 14 -> 15 -> $10A10

Special destructible:
  23 -> 24 -> 25 -> 26
```

## Next target

Trace dynamic creation of state 23, while decoding state 8 and state 14 far enough to attach their cast names confidently. The state-12 subtype `0x66` and default branch are also now compact targets for identifying more of the manual's cast.
