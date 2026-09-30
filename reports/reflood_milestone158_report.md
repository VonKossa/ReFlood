# ReFlood Milestone 158: larger proportional split view and brown actors

## Changes

- The centered black divider is now two logical pixels wide, double milestone 157.
- The extra player and controller headers and the added status footer have been removed. Each side shows only the original in-game HUD, including its own score and lives and the shared trash count.
- Each side draws a centered 240×200 source playfield at 318×265 logical pixels (both ratios are exactly 1.2), with the entire original 320×8 HUD strip separately drawn above it. This uses more vertical screen area without stretching the gameplay. It reduces horizontal world visibility from 320 to 240 source pixels. Both Nearest and Scale2x SDL source coordinates account for their texture sizes.
- Quiffy 2 and Matilda 2 share a brown palette ramp. Quiffy 2 preserves the original cool and cream goggle colors. Matilda 2's two eyes are bright red in each of the eight normal facing and animation frames. The recoloring occurs at render time; the source sprite assets are unchanged.
- Individual player scores and the shared trash count from milestone 157 remain in the actor and world state respectively. Multiplayer display and palette work remain in the dedicated multiplayer files and small renderer hooks.

## Verification

- Strict C99 core and multiplayer tests: pass, including pickup ownership, the shared trash counter, reset persistence, friendly fire, exits, survivor play, and a frame on all 42 extracted levels.
- SDL input/settings stub test: pass, including source sprite checks for Quiffy 2's goggle colors and Matilda 2's red eyes in eight frames.
- UndefinedBehaviorSanitizer core and multiplayer tests: pass.
- Strict real SDL2 GCC build: pass. SDL dummy video/audio software renderer: both enlarged panels populated, four physical black pixels corresponding to two logical divider pixels at 1280×720, both HUD strips visible with separate scores and shared trash, and a red Matilda eye pixel. Nearest and Scale2x variants started and exited a multiplayer session cleanly. The Nearest frame was inspected visually.
- The source package contains no extracted game data or compiled binaries; `data/` is empty.

## Experimental limits

The centered crop is the tradeoff for using more of the 16:9 split while preserving the original pixel aspect: 40 source pixels on either side are outside each player's view. A full 320-pixel wide view would have to remain 208 pixels high at this split width, or be stretched. Enemy targeting still follows the experimental shared dispatcher described in milestone 156. Physical controllers, a GPU renderer, and a real display were not available for testing here.
