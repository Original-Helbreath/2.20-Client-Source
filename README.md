# 2.20-Client-Source

Source code of the Helbreath 2.20 client (Visual C++ 6, DirectDraw / DirectInput / DirectSound).

## Contents

Transferred from the public librehelbreath copy of the leaked 2.20 client source.
There is no project file yet, and the client hard-codes its log server address; both are on the project board.
Prebuilt DLLs and `.lib` link libraries that came with the source tree are kept in [2.20-Reference](https://github.com/Original-Helbreath/2.20-Reference);
the build needs them replaced by SDK libraries or by their own sources (see the project board).

| Folder | What it holds |
|---|---|
| `Client/` | The client source (Visual C++ 6, DirectDraw / DirectInput / DirectSound). |
| `Libraries/Search/` | search.dll, the anti-cheat DLL: documentation and decompiles of both builds only. The client no longer loads it. |
| `Libraries/CInt/` | Cint.lib (`CInt`, the hidden-int class): documentation and disassembly only for now; the client still uses `Client/Cint.h` with the prebuilt .lib. |
| `Libraries/StrTok/` | `CStrTok` tokenizer replacing StrTok.lib and the client's old in-tree copy. |

## Branches

| Branch | Purpose |
|---|---|
| `original` | The files exactly as found. Read-only by convention: never commit here. (It is not protected yet, because the org is on GitHub Free and its repos are private.) |
| `development` | Integration branch. Work happens in feature branches, which merge into `development`. |
| `stable` | Default branch. `development` is merged here once it is stable. |

## Project

Part of [Original Helbreath](https://github.com/Original-Helbreath): keeping the original 1999-2005 Siementech Helbreath alive.
Repos are named `<version>-<Part>`; other versions follow the same pattern.
Issues are tracked on the [project board](https://github.com/Original-Helbreath/projects/1). The analysis behind everything is in
[2.20-Documentation](https://github.com/Original-Helbreath/2.20-Documentation).
