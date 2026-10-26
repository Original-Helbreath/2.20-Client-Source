# Client data formats

The client reads its data files as JSON (`Client/ClientData.cpp`, using nlohmann/json from
`External/nlohmann/`). They replace the text files that the client used to tokenise with the
`CStrTok` class, which has been removed. The JSON files are produced from the original text files
by `client-data/convert_client_data.py` in the [Tools](https://github.com/Original-Helbreath/Tools) repo;
the converted files are on the `feature/json-data` branch of 2.20-Client-Files (the originals stay on `original`).

**Intent:** each JSON file holds exactly the data the old loader built in memory from the text file,
already parsed. The client no longer tokenises anything; the old parsing rules (prefix tests, the
tokenizer's handling of blank lines and separators, `atoi`) were applied once, by the converter.

## Contents

- Common envelope
- One section per format: `contents`, `game-messages`, `build-items`, `magic-config`, `skill-config`, `bad-words`, `login-config`, `curse-words`
- Behaviour of the loaders
- Equivalence check
- Typed chat commands and the command line

## Common envelope

Every file is a JSON object (UTF-8, no BOM) with:

| Key | Type | Meaning |
|---|---|---|
| `format` | string | The format name below. The loader rejects any other value. |
| `version` | integer | `1`. The loader rejects any other value. |
| `encoding` | string | The original text file's encoding: `ascii`, `cp949`, `cp1252`, `latin-1` (or `cp932`). The client converts every string back to this code page (949, 1252, 28591, 932; ascii is checked to be 7-bit), so its in-memory strings are byte-for-byte the ones it read from the text file. A string that cannot be converted without substitution makes the file invalid. |
| `source` | object | Provenance, not read by the client: `file` (original name), `size`, `sha256` of the original bytes, and for the XOR-obfuscated files `xor` (`header` or `headerless`). |

Integers are the `atoi()` value of the original token, so a field the original file left
non-numeric is stored as what the client computed from it.

## Formats

### `contents` - `contents\contents<N>.json`

Replaces `contents\contents<N>.txt` (any N). Read by the text dialogs (`_LoadTextDlgContents`,
`_iLoadTextDlgContents2`), the agreement screen (`_LoadAgreementTextContents`) and the shop menu
(`_LoadShopMenuContents`); which one reads a given N is decided at run time (by the server for shops),
so one schema serves all of them.

| Key | Type | Meaning |
|---|---|---|
| `lines` | array of string | The text lines, as the old tokenizer returned them (separator `\n`): empty lines are not stored, except that a file starting with an empty line has `""` first. |
| `shop` | object, optional | Present when the file is a shop list. `items`: array of item objects. |

Shop item object (one per `Item = ...` line; `name` and every integer below are required, the
columns the client never read are kept for completeness):

| Key | Client field | Key | Client field |
|---|---|---|---|
| `name` | `CItem::m_cName` (max 20 bytes) | `max_life_span` | `m_wMaxLifeSpan` |
| `item_type` | `m_cItemType` | `max_fix_count` | not read |
| `equip_pos` | `m_cEquipPos` | `sprite` | `m_sSprite` |
| `effect_type` | not read | `sprite_frame` | `m_sSpriteFrame` |
| `effect_value1` .. `effect_value6` | `m_sItemEffectValue1..6` | `price` | `m_wPrice` |
| `appr_value` | not read | `weight` | `m_wWeight` |
| `speed` | `m_cSpeed` | `level_limit` | `m_sLevelLimit` |

`m_dwCount` is set to 1 for every item, as before.

### `game-messages` - `contents\GameMsgList.json`

`messages`: array of string, the tokens of `GameMsgList.txt` with separators `;` and `\n`.
At most 300 (`DEF_MAXGAMEMSGS`) are loaded.

### `build-items` - `contents\BItemcfg.json`

`items`: array of objects (at most 100, `DEF_MAXBUILDITEMS`; more makes the file invalid):

| Key | Client field |
|---|---|
| `name` | `CBuildItem::m_cName` (max 20 bytes) |
| `skill_limit` | `m_iSkillLimit` |
| `elements` | exactly 6 objects `{name, count}` -> `m_cElementName1..6` (max 20 bytes) and `m_iElementCount[1..6]` |
| `sprite`, `sprite_frame` | `m_iSprH`, `m_iSprFrame` |
| `max_skill` | `m_iMaxSkill` |

### `magic-config` - `contents\magiccfg.json`

`magic`: array of `{id, name, value1, value2, value3, value4, value5, value6, visible}`. `id`
(0..99) indexes `m_pMagicCfgList`; a duplicate or out-of-range id makes the client's init fail.
`name` max 30 bytes. `value1..3` -> `m_sValue1..3`; `value4..6` are not read (the old loader skipped
them too). `visible` (boolean) -> `m_bIsVisible`.

### `skill-config` - `contents\skillcfg.json`

`skills`: array of `{id, name, useable, use_method}`. `id` (0..59) indexes `m_pSkillCfgList`; a
duplicate or out-of-range id makes init fail. `name` max 20 bytes. `useable` -> `m_bIsUseable`,
`use_method` -> `m_cUseMethod`.

### `bad-words` - `contents\badword.json`

`words`: array of string, the tokens of the decoded `badword.txt` with separators `/ , \t \n`. At
most 500 (`DEF_MAXBADWORD`) are loaded. `bCheckImportantFile` requires this file to exist, and an
invalid file stops the client at start-up with an error box, as a bad `badword.txt` did.

**The XOR obfuscation is gone.** `badword.txt` was XORed with key 35 (and, in the 2.20 form, prefixed
by a 10-byte size header XORed with 20); the client decoded it into a temporary `badword.tmp` and
deleted that after reading. The JSON file is plain text, so there is no decode step and no
temporary file. The converter records which XOR form the original used in `source.xor`. The
`badword.txt` shipped in 2.20-Client-Files has the older headerless form; the 2.20 client's own
decoder would have rejected it (its header check fails), so with that file the old client stopped at
start-up, while the JSON file loads.

### `login-config` - `login.json`

Optional keys `log_server_address` (string, max 15 characters) and `log_server_port` (non-zero
integer), each assigned only when present, as with the keys of `LOGIN.CFG`. Read only when the client
is built with `DEF_LANGUAGE` 3 or lower; the current setting (`DEF_LANGUAGE 4`) compiles in a fixed
address instead and does not read the file at all (unchanged).

### `curse-words` - converter only

`lines`: the lines of the decoded `Curse.txt` that `CCurse::LoadCurse` keeps (not starting with `$`
or `;`, not empty), without their line feed. The client's `CCurse::LoadCurse` did not use `CStrTok`
and still reads the XORed `Curse.txt`; this format exists so the file can be converted when that
loader moves to JSON. No `Curse.txt` is in 2.20-Client-Files.

## Behaviour of the loaders

Each loader keeps the old reaction to a missing file and applies it to an invalid JSON file too:
text dialogs, agreement and game messages load nothing; `_iLoadTextDlgContents2` returns -1; build
items, magic, skill and login config return FALSE (init fails with the same message box, now naming
the `.json` file); the bad-word list fails init.

Where the old code had no bound and could write past a fixed array, the new code stops at the
array's size instead (the output for in-range data is unchanged):

- text dialog lines are capped at 300 (`DEF_TEXTDLGMAXLINES`). The shipped `contents10.txt` has 443
  lines, so the old loader wrote past `m_pMsgTextList`; the client now keeps the first 300;
- game messages at 300, build items at 100 (more fails), magic/skill ids outside their tables fail;
- names are copied into their fixed `char` arrays with truncation at the array size (the converter
  refuses names that would not fit, so shipped data is never truncated);
- command-line tokens are copied with truncation at 119 characters.

## Equivalence check

For every converted file in 2.20-Client-Files (`development`, 55 files), the in-memory result of the
old text path was compared with that of the new JSON path:

- **Old path:** a Python model of each old loader (MSVC text-mode read, `CStrTok`, the loader's read
  modes, `atoi`), run on the original text file. The `CStrTok` model and the client's new
  `SplitTokens` were each compared with the reviewed `CStrTok` C++ class (built with MSVC) on 100,000
  random inputs: 0 mismatches for both.
- **New path:** `Client/ClientData.cpp` built with MSVC (C++17) into a small dump program, run on the
  converted JSON files. Its strings went through the real Win32 code-page conversion.

Result: 53 of 55 files identical, field by field and byte by byte (all text dialogs, the shop list in
`contents1`/`contents2`, game messages, build items, magic, skills, bad words, login). The 2 differences
are intended: in `contents0.txt` and `contents907.txt`, prose words beginning with `Item` would have
been parsed as 2 and 1 garbage shop items **if** the server had ever opened those files as a shop; they
are text files, so they get no `shop` section. Their `lines` are identical.

The mapping from the parsed structs to the client's objects (`CItem`, `CBuildItem`, `CMagic`, `CSkill`,
`CMsg`) in `Game.cpp`/`GameMonitor.cpp` was checked by review; `Game.cpp` and `GameMonitor.cpp` pass
an MSVC C++17 compile without code generation (`cl /Zs`). The client as a whole is not built or run:
there is no project file yet.

## Typed chat commands and the command line

These are not files. `Client/StrSplit.h` provides `SplitTokens(data, seps)`, a stateless C++17
function returning `std::vector<std::string_view>` with exactly the tokens `CStrTok` returned
(including its quirks: a leading separator gives an empty first token, `\r\n` always separates).
It is used by `CGame::bInit` (command line, separators `& = , space tab LF`), `CGame::OnKeyUp`
(Ctrl+T: the sender name before `:` in a chat line) and `CGame::bCheckLocalChatCommand` (`/tooff`,
`/toon`: the second token).
