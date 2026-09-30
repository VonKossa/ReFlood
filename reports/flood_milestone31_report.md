# Flood reverse engineering — Milestone 31

## Main result: Sparkling Fungi are not a generic terrain hazard

Two independent audits now converge:

1. Milestone 30 showed that the 256-byte tile-attribute table has no generic lethal flag; its extra bits are collision/geometry modifiers.
2. This milestone enumerated every direct call to the common player overlap routine `$10DCA`. All 21 calls belong to player/history logic, known runtime enemies, Mine/Heart paths, weapons/effects, or special mechanisms. There is no ordinary terrain-tile instant-kill caller.

Contemporary documentation describes Sparkling Fungi as stationary objects that kill Quiffy on contact. The binary evidence now says they are almost certainly represented as a placed **static/special object** rather than ordinary terrain metadata.

## Marker 7 false lead removed

Marker 7 was worth revisiting because it appears only four times in the raw 42-level dataset, close to the manual's 'quite rare' description. Exact inspection of the `$E7B0` jump table rules it out: marker 7 does not allocate a runtime object; it is cleared as setup data.

The special-marker layer therefore still has no credible Fungi candidate.

## The `C8/CA` mechanism family is not Fungi

The player-contact helpers near `$13336` and `$13588` interact with map bytes `$CA` and `$C8`. Both use `$10DCA`, but contact subtracts only 16 Life Force and updates mechanism state globals. This finite-damage mechanical family does not match the instant-lethal Fungi description.

## Updated search model

The most likely implementation is now one of:

- a secondary static-object table separate from the 128-record runtime creature table;
- an ordinary map tile discovered by a later conversion/scanning pass and represented in another object structure;
- a dormant/indirect runtime state not created by the primary marker initializer.

The highest-value static targets are the object/map conversion routines around `$128CC/$1291A/$12E78` and the secondary structures referenced around `$1502E/$150FC/$152DA/$15596`.

## State 23

Nothing in this audit changes the earlier runtime-state-23 conclusion. Marker 23 is camera/bounds setup and marker-driven triggers cannot directly create runtime behavior 23. The WinUAE value-filtered watchpoint remains the best dynamic confirmation path.
