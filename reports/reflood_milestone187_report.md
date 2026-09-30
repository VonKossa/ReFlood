# ReFlood Milestone 187: Editor keyboard focus

ReFlood-Editor now requests SDL's keyboard grab while its window has focus and releases it on focus loss and exit. This lets the editor receive shortcuts such as Ctrl+S before XFCE's window-manager bindings, in both windowed and fullscreen modes. The mouse remains free. SDL2 2.0.16 or later is required for the keyboard-grab API; on older SDL2 builds the editor retains its previous behavior. SDL's default fullscreen Alt+Tab escape remains enabled.

The README documents the behavior and limits. No game code or extracted assets changed.

Validation: inspected the focus event handling and package contents. An XFCE desktop session and SDL2 development libraries are unavailable in this environment, so this interaction needs a runtime check on the user's desktop.
