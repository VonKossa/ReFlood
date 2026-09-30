# Flood Milestone 81: controls and fullscreen

## Result

Milestone 81 moves the fire/confirm control from Space to **Right Ctrl** and
adds an **F** key toggle for SDL borderless-desktop fullscreen.

SDL continues to render through the logical size selected for each original
screen. When fullscreen is active, SDL scales that logical image to the largest
fit while preserving the source aspect ratio and letterboxes any unused area.
The toggle is edge-triggered, so keyboard-repeat events cannot make fullscreen
oscillate while F is held.

## Text-entry exception

The level selector and copy-protection challenge both accept alphabetic input.
On those two screens, F remains a typed `F` rather than toggling fullscreen;
otherwise valid codes or poster answers containing F would be impossible.
Fullscreen can be toggled on every non-text-entry screen before or after them.
Space remains available within multiword copy-protection answers, but is no
longer a fire or confirmation control.

## Verification

The full strict-C99 and UndefinedBehaviorSanitizer core suites pass. Normal and
sanitized headless hosts complete all 42 cavern smoke runs, and the SDL host
path passes a strict C99 syntax build against a local SDL2 interface stub that
includes the fullscreen and Right Ctrl APIs used here.
