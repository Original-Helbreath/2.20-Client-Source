# nlohmann/json (vendored)

| | |
|---|---|
| Library | JSON for Modern C++ (nlohmann/json), single header |
| Version | 3.11.3 |
| Release | https://github.com/nlohmann/json/releases/tag/v3.11.3 |
| License | MIT (`LICENSE.MIT`) |

Files, unmodified:

| File | Downloaded from | SHA-256 |
|---|---|---|
| `json.hpp` | https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp (release asset) | `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6` |
| `LICENSE.MIT` | https://raw.githubusercontent.com/nlohmann/json/v3.11.3/LICENSE.MIT (the v3.11.3 tag; the release has no separate license asset) | `86b998c792894ccb911a1cb7994f7a9652894e7a094c0b5e45be2f553f45cf14` |

Used by `Client/ClientData.cpp` as `#include <nlohmann/json.hpp>`, so `External/` must be on the
include path. Requires C++11 or later; the client targets C++17.
