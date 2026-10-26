# StrTok

`CStrTok` splits a text buffer into tokens. The client uses it to parse its command line and its
config, message and bad-word files (`Game.cpp`, `GameMonitor.cpp`).

## Three versions

| Version | Token buffer | `_bIsSeperator` | Notes |
|---|---|---|---|
| `StrTok.lib` (2.20-Reference `libraries/client-source/`, 2,742 bytes) | `char[1024]` | private, `BOOL` | What the shipped client linked. |
| Game server `StrTok.cpp/.h` (`2.20-Game-Server-Source`) | `char[1024]` | private, `BOOL` | Identical to the .lib. |
| Client's old in-tree `StrTok.cpp/.h` | `char[256]` | public, `bool` | Same tokens; branches written the other way round. |

**The .lib matches the server source.** The .lib's single member `.\Release\StrTok.obj` has the same
symbols as the server class: ctor, virtual dtor, `pGet`, and private `_bIsSeperator` returning `BOOL`.
Its code matches as well: the constructor sets `m_iDataLength` at offset 0x40c and `m_iCurLoc` at 0x410,
which means a 1024-byte token buffer, and `pGet` zeroes 0x400 bytes and tests `== TRUE` where the
server source does.

The client's in-tree copy produces the same tokens. It only differs in buffer size: tokens of 256
characters or more overflowed it, which the .lib allows up to 1024.

## This replacement

`StrTok.h` / `StrTok.cpp` here replace both the .lib and the in-tree copy:

- **Same interface:** the `CStrTok(char *pData, char *pSeps)` constructor, the virtual destructor,
  `char *pGet()`, and the public `m_pData`, `m_pSeps`, `m_iDataLength` and `m_iCurLoc`. The client only
  ever calls the constructor, `pGet()` and `delete` (checked against every use in `Game.cpp` and
  `GameMonitor.cpp`).
- **Same tokens:** the rules are listed at the top of `StrTok.cpp`. That includes the quirks: `"\r\n"`
  counts as a separator, leading separators give one empty `""` token, two or more trailing separators
  give one empty token, and NULL is returned only at the end.
- **No fixed buffer:** the token lives in a `std::vector<char>` owned by the object, so long tokens are
  neither truncated nor overrun. As before, the returned pointer is valid until the next `pGet()`.
  There is no static state. The
  buffer reserves 1024 bytes up front, so the returned pointer keeps the same address for every token up
  to the original buffer size; only a longer token moves it.
- **Verified:** a differential test ran 200,000 random inputs through the server/.lib source and through
  this class (token lengths under the 1024 limit), with 0 mismatches. It was built with current MSVC,
  not VC6.

### Toolchain

The client is a Visual C++ 6 project. VC6 predates C++11, so `std::string_view`, `auto`, lambdas and the
other modern features cannot be used without dropping VC6. This class uses only C++98 and the STL
(`std::vector`), which VC6 supports. Building it with VC6 itself is **unverified**: no VC6 is installed
here.

The client's in-tree `StrTok.cpp/.h` were removed. `Client/Game.h` still has `#include "StrTok.h"`,
so `Libraries/StrTok/` must be on the include path once a project file exists.
