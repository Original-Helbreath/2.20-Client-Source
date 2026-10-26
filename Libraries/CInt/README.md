# CInt

Source for `Cint.lib` (6,386 bytes, in 2.20-Reference `libraries/client-source/`), recreated from its
object code. The client uses `CInt` in `CharInfo.h` and `Game.h` for values it wants hidden from memory
scanners. The comment in CharInfo.h says this was to stop characters entering at level 0 (2003-02-20).

## What it does

`CInt` stores an `int` outside the object that owns it, scrambled with a key:

- The **constructor** commits its own 4-byte block (`VirtualAlloc`, `MEM_COMMIT`, `PAGE_READWRITE`),
  picks a key with `rand()`, and stores `0 ^ key`.
- **`Set(v)`** / **`operator=`** first makes the block writable (`VirtualProtect` -> `PAGE_READWRITE`). If
  that fails, nothing changes. Otherwise it picks a **new** `rand()` key, stores `v ^ key`, and makes the
  block `PAGE_READONLY` again. A stray write to the block therefore faults.
- **`Get()`** / **`operator int()`** return `*m_pValue ^ m_iKeyValue`.
- The **destructor** calls `VirtualFree(..., MEM_DECOMMIT)`. It does not release the reservation, and
  this is kept as-is.

The original has no NULL check after `VirtualAlloc`, and neither does the recreation. The key comes from
`rand()`, so it shares the CRT seed the client sets with `srand(time(NULL))` in `WinMain`.

## How the .lib was read

The .lib is an `ar` archive with one member, `.\Release\Cint.obj`, compiled by VC6 (`@comp.id` 0xb2306).
The member was extracted, and its COMDAT `.text` sections were listed with relocations resolved to
symbols. They are small enough to read by hand, so no Ghidra import was needed. The annotated
disassembly and pseudo-C are in `decompile/Cint.c`. All symbols match the methods declared in `Cint.h`.

## `==` and `!=`

The .lib also defines `bool CInt::operator==(const int &)` and `operator!=`, both `Get()` compared with
the argument. In `Cint.h` their declarations are commented out, so `src/CInt.cpp` keeps their bodies
commented out too. Restore both together if they are ever needed.

## Files

- `src/CInt.cpp`: the recreated implementation.
- `decompile/Cint.c`: hand disassembly.

The header stays at `Client/Cint.h`. The client includes it as `"Cint.h"` from `CharInfo.h` and
`Game.h`, and there is no project file yet to add an include path, so moving it would break those
includes. Build `src/CInt.cpp` with `Client/` on the include path.
