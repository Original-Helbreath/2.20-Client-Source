# 2.20-Client-Source

Source code of the Helbreath 2.20 client (DirectDraw / DirectInput / DirectSound). Originally a Visual C++ 6 project; the target is now modern MSVC with C++17.

## Contents

Transferred from the public librehelbreath copy of the leaked 2.20 client source.
There is no project file yet, and the client hard-codes its log server address; both are on the project board.
Prebuilt DLLs and `.lib` link libraries that came with the source tree are kept in [2.20-Reference](https://github.com/Original-Helbreath/2.20-Reference);
the build needs them replaced by SDK libraries or by their own sources (see the project board).

The client reads its data files (contents, shop lists, crafting recipes, magic and skill tables, bad-word list, login config) as JSON;
the `CStrTok` tokenizer (StrTok.lib) is gone. The formats are in [docs/data-formats.md](docs/data-formats.md); the converter from the original
text files is `client-data/` in [Tools](https://github.com/Original-Helbreath/Tools). Building needs `External/` on the include path and C++17.

| Folder | What it holds |
|---|---|
| `Client/` | The client source (DirectDraw / DirectInput / DirectSound). |
| `External/nlohmann/` | nlohmann/json 3.11.3 (`json.hpp`), vendored unmodified; version, source URL and SHA-256 in its README. |
| `docs/` | `data-formats.md`: the JSON schemas of the client data files and how they were checked against the old text loaders. |

search.dll (the anti-cheat check) and Cint.lib (`CInt`) are no longer used by the client; what they did is documented in [2.20-Reference](https://github.com/Original-Helbreath/2.20-Reference) under `decompile/`.

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
