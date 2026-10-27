// ClientData.h: readers for the client's JSON data files.
//
// Each reader opens one JSON file (schemas: docs/data-formats.md), checks its "format" and
// "version", and returns plain C++ data. Strings come back in the file's recorded original
// encoding (e.g. cp949 bytes), which is what the rest of the client works with. A reader returns
// false when the file is missing or is not a valid document of its format; the callers keep the
// old loaders' reactions to a missing file.
//
// This file has no dependency on the rest of the client, so it can be built and tested alone.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <string>
#include <vector>

namespace ClientData {

// format "contents" -> "lines"; "game-messages" -> "messages"; "bad-words" -> "words".
bool ReadStringList(const char * pPath, const char * pFormat, const char * pKey,
                    std::vector<std::string> & out);

// format "contents", optional "shop" section. bHasShop is false when the file has none.
struct ShopItem {
	std::string name;
	int itemType, equipPos;
	int effectValue1, effectValue2, effectValue3, effectValue4, effectValue5, effectValue6;
	int maxLifeSpan, sprite, spriteFrame, price, weight, speed, levelLimit;
};
bool ReadShopItems(const char * pPath, std::vector<ShopItem> & out, bool & bHasShop);

// format "build-items".
struct BuildElement {
	std::string name;
	int count;
};
struct BuildItem {
	std::string name;
	int skillLimit;
	std::array<BuildElement, 6> elements;
	int sprite, spriteFrame, maxSkill;
};
bool ReadBuildItems(const char * pPath, std::vector<BuildItem> & out);

// format "magic-config".
struct MagicCfg {
	int id;
	std::string name;
	int value1, value2, value3;
	bool visible;
};
bool ReadMagicConfig(const char * pPath, std::vector<MagicCfg> & out);

// format "skill-config".
struct SkillCfg {
	int id;
	std::string name;
	int useable, useMethod;
};
bool ReadSkillConfig(const char * pPath, std::vector<SkillCfg> & out);

// format "login-config". Each value is optional, as each key was in LOGIN.CFG.
struct LoginCfg {
	bool hasAddress = false;
	std::string address;
	bool hasPort = false;
	int port = 0;
};
// Absent: there is no login.json at all (the only case that may keep built-in defaults).
// Invalid: the file exists but is malformed, has the wrong format, a field of the wrong type, an empty
// address or a port outside 1..65535; out is left empty. A caller must not fall back on invalid.
enum class LoginConfigStatus { Ok, Absent, Invalid };
LoginConfigStatus ReadLoginConfig(const char * pPath, LoginCfg & out);

// format "client-config" (client.json; // and /* */ comments allowed). Client settings that are not
// part of the 2.20 data. A missing or invalid file leaves the defaults, which are windowed.
struct ClientCfg {
	bool windowed = true;   // false: exclusive full-screen 640x480 16-bit, as the 2.20 Release build
};
bool ReadClientConfig(const char * pPath, ClientCfg & out);

// The server notice (contents\contents1000.json). WriteNoticeContents stores pText, the notice
// as the server sent it (a C string in the client's code page), as a "contents" document whose
// "lines" are exactly what the old path gave: the text written to contents1000.txt in text mode,
// read back in text mode and split on "\n" by CStrTok. "source.size" records the size the old
// contents1000.txt would have had on disk; ReadNoticeSourceSize returns it (0 when the file is
// missing or invalid), because the client reports that size to the server to ask whether the
// notice changed.
bool WriteNoticeContents(const char * pPath, const char * pText);
unsigned long ReadNoticeSourceSize(const char * pPath);

// Copies src into a fixed char array of the given size: zero-filled, at most size-1 bytes.
void CopyToBuffer(char * pDst, std::size_t size, const std::string & src);

} // namespace ClientData
