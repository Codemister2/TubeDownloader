# Turbo Ball Arena 4.0 — Direct Render Edition

This rebuild removes the render-texture and post-processing pipeline that caused the in-game black screen on some Windows GPUs.

## Start

1. Extract the ZIP into a new folder.
2. Double-click `AUTO_INSTALL_AND_PLAY.bat`.
3. Allow Windows Package Manager to install missing build requirements when prompted.

Use `CLEAN_REBUILD_AND_PLAY.bat` if an older build cache exists.

## Controls

- `WASD`: Drive and steer
- `Space`: Jump; press again in the air for a directional dodge
- `Left Shift`: Boost
- `Q` / `E`: Air roll
- `C`: Toggle Ball Cam
- `P`: Pause or resume
- `Esc`: Exit the match to the main menu

Gamepad:

- Left stick: Drive, steer, and aerial control
- A: Jump and dodge
- Right trigger: Boost

## Visibility fixes

- Direct 3D rendering to the window
- No render textures during gameplay
- No post-processing shader during gameplay
- No retained off-screen depth buffer
- Windows Segoe UI font when available
- Clean HUD with score, clock, boost, and camera mode
- Resizable 1600×900 window with 960×540 minimum

## Requirements

The automatic launcher installs or detects:

- Git
- CMake
- Visual Studio 2022 C++ Build Tools
- raylib 6.0, downloaded by CMake from the official raylib repository
