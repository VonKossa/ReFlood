# ReFlood Milestone 162: cross-Matilda hurt voice

## Change

Continuous contact with the other player's Matilda now plays one hurt voice on entry. Both players still take the original 10 Life Force damage per overlapping frame; the voice plays again after contact ends and starts anew. The first native damage-feedback pass had cleared the contact latch before the cross-ghost check. The multiplayer pass now retains the incoming latch for that check and avoids a second feedback call if the native Matilda already made contact. The behavior is symmetric for Player 1 and Player 2.

The change is confined to the multiplayer simulation include, its regression test, and the README. No game data or copyrighted assets are included.

## Verification

- Strict C99 original core and multiplayer core tests passed.
- The multiplayer regression checks both cross contacts, three sustained damage frames without repeated voice IDs, one separated frame, then a fresh voice on recontact.
- SDL input/settings stub suite and strict real SDL2 build passed.
- UndefinedBehaviorSanitizer multiplayer suite passed.

The voice was checked through the sound queue and not listened to on a physical audio device. Existing experimental multiplayer limitations remain as documented in the README and earlier reports. The source ZIP contains an empty `data/` directory and fixed 2025-01-01 entry timestamps to avoid clock-skew warnings after extraction.
