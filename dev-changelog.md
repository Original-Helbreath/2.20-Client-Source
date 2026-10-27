## Build the client from the tree

### Client

**Added**
- `client.sln` and `Client\client.vcxproj` (VS 2026, v145, C++20, x64 Debug and Release, output `build\x64\<cfg>\`), so the 2.20 client builds from the tree.

**Fixed**
- The client loads `DirectInputCreateA` from `System32\dinput.dll` only, at run time, because the current Windows SDK no longer ships `dinput.lib`.
- Three breaks on the current compiler: an old-style loop variable in `Misc.cpp`, the removed `<iostream.h>` in `SoundBuffer.cpp`, and the `timeSetEvent` callback signature (`DWORD_PTR`) in `Wmain.cpp` / `Winmain.h`.

Summary: Client gains a C++20 x64 project that builds the 2.20 client with three forced compile fixes and a System32-only DirectInput load (2026-10-27)
