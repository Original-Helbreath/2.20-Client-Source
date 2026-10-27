## Run the client in a window

### Client

**Added**
- The client runs in a normal 640x480 window, chosen by `"windowed"` in the new `Client\client.json` (comments allowed; a missing or invalid file means windowed). This replaces the compile-time Debug/Release switch.

**Changed**
- Back buffer and sprite surfaces are always 16-bit 565, and windowed frames are copied to the window with `StretchDIBits`, so colours are correct on 32-bit desktops.
- In windowed mode the mouse is shared with Windows: the cursor follows the system cursor in window coordinates, and title-bar clicks do not reach the game.
- In windowed mode the game keeps drawing while another window has focus, repaints when uncovered, and Alt no longer opens the window menu and freezes the game.
- Debug builds no longer switch to full-screen when Enter is pressed; `/enabletogglescreen` still enables the toggle.

**Fixed**
- Sprite surface locks on x64 used the 32-bit size of the DirectDraw surface description (`124`), so every lock failed and the screen stayed black; they use `sizeof(ddsd)`.

Summary: Client runs windowed from a JSON setting with correct colours and a shared mouse, and draws on x64 (2026-10-27)
