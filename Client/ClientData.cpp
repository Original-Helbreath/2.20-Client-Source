// ClientData.cpp: readers for the client's JSON data files (see ClientData.h and
// docs/data-formats.md). Needs External/ on the include path for nlohmann/json.hpp.
//////////////////////////////////////////////////////////////////////

#include "ClientData.h"
#include "StrSplit.h"

#include <windows.h>

#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>

namespace ClientData {

namespace {

using nlohmann::json;

constexpr int kFormatVersion = 1;

// Windows code page for each "encoding" value the converter writes.
bool CodePageOf(const std::string & enc, UINT & cp)
{
	if (enc == "ascii")   { cp = 20127; return true; }
	if (enc == "cp949")   { cp = 949;   return true; }
	if (enc == "cp1252")  { cp = 1252;  return true; }
	if (enc == "latin-1") { cp = 28591; return true; }
	if (enc == "cp932")   { cp = 932;   return true; }
	return false;
}

// A loaded document plus the code page its strings convert back to.
struct Doc {
	json j;
	UINT cp = 0;
};

bool Load(const char * pPath, const char * pFormat, Doc & doc)
{
	std::error_code ec;
	if (!std::filesystem::is_regular_file(std::filesystem::u8path(pPath), ec)) return false;

	std::ifstream in(std::filesystem::u8path(pPath), std::ios::binary);
	if (!in) return false;
	std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	doc.j = json::parse(text, nullptr, false);   // no exceptions: invalid JSON is "discarded"
	if (doc.j.is_discarded() || !doc.j.is_object()) return false;

	const auto fmt = doc.j.find("format");
	const auto ver = doc.j.find("version");
	const auto enc = doc.j.find("encoding");
	if (fmt == doc.j.end() || !fmt->is_string() || *fmt != pFormat) return false;
	if (ver == doc.j.end() || !ver->is_number_integer() || *ver != kFormatVersion) return false;
	if (enc == doc.j.end() || !enc->is_string()) return false;
	return CodePageOf(enc->get<std::string>(), doc.cp);
}

// UTF-8 JSON string -> bytes in the document's original encoding. Fails rather than substitute.
bool ToClientString(const json & v, UINT cp, std::string & out)
{
	if (!v.is_string()) return false;
	const std::string & u8 = v.get_ref<const std::string &>();
	out.clear();
	if (u8.empty()) return true;

	if (cp == 20127) {   // ascii: the bytes are already right
		for (unsigned char c : u8) if (c >= 0x80) return false;
		out = u8;
		return true;
	}

	const int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8.data(), (int)u8.size(), nullptr, 0);
	if (wlen <= 0) return false;
	std::wstring w(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8.data(), (int)u8.size(), &w[0], wlen);

	BOOL bUsedDefault = FALSE;
	const int len = WideCharToMultiByte(cp, WC_NO_BEST_FIT_CHARS, w.data(), wlen, nullptr, 0, nullptr, &bUsedDefault);
	if (len <= 0 || bUsedDefault) return false;
	out.resize(len);
	WideCharToMultiByte(cp, WC_NO_BEST_FIT_CHARS, w.data(), wlen, &out[0], len, nullptr, &bUsedDefault);
	return !bUsedDefault;
}

bool GetInt(const json & obj, const char * pKey, int & out)
{
	const auto it = obj.find(pKey);
	if (it == obj.end() || !it->is_number_integer()) return false;
	const auto v = it->get<long long>();
	if (v < INT_MIN || v > INT_MAX) return false;
	out = (int)v;
	return true;
}

bool GetString(const json & obj, const char * pKey, UINT cp, std::string & out)
{
	const auto it = obj.find(pKey);
	return it != obj.end() && ToClientString(*it, cp, out);
}

const json * GetArray(const json & obj, const char * pKey)
{
	const auto it = obj.find(pKey);
	return (it != obj.end() && it->is_array()) ? &*it : nullptr;
}

} // namespace

bool ReadStringList(const char * pPath, const char * pFormat, const char * pKey,
                    std::vector<std::string> & out)
{
	out.clear();
	Doc doc;
	if (!Load(pPath, pFormat, doc)) return false;
	const json * arr = GetArray(doc.j, pKey);
	if (arr == nullptr) return false;
	for (const json & v : *arr) {
		std::string s;
		if (!ToClientString(v, doc.cp, s)) { out.clear(); return false; }
		out.push_back(std::move(s));
	}
	return true;
}

bool ReadShopItems(const char * pPath, std::vector<ShopItem> & out, bool & bHasShop)
{
	out.clear();
	bHasShop = false;
	Doc doc;
	if (!Load(pPath, "contents", doc)) return false;
	const auto shop = doc.j.find("shop");
	if (shop == doc.j.end()) return true;            // a text-only contents file: no items
	if (!shop->is_object()) return false;
	const json * items = GetArray(*shop, "items");
	if (items == nullptr) return false;
	bHasShop = true;
	for (const json & o : *items) {
		ShopItem it;
		if (!o.is_object() || !GetString(o, "name", doc.cp, it.name)
		    || !GetInt(o, "item_type", it.itemType) || !GetInt(o, "equip_pos", it.equipPos)
		    || !GetInt(o, "effect_value1", it.effectValue1) || !GetInt(o, "effect_value2", it.effectValue2)
		    || !GetInt(o, "effect_value3", it.effectValue3) || !GetInt(o, "effect_value4", it.effectValue4)
		    || !GetInt(o, "effect_value5", it.effectValue5) || !GetInt(o, "effect_value6", it.effectValue6)
		    || !GetInt(o, "max_life_span", it.maxLifeSpan) || !GetInt(o, "sprite", it.sprite)
		    || !GetInt(o, "sprite_frame", it.spriteFrame) || !GetInt(o, "price", it.price)
		    || !GetInt(o, "weight", it.weight) || !GetInt(o, "speed", it.speed)
		    || !GetInt(o, "level_limit", it.levelLimit)) {
			out.clear();
			return false;
		}
		out.push_back(std::move(it));
	}
	return true;
}

bool ReadBuildItems(const char * pPath, std::vector<BuildItem> & out)
{
	out.clear();
	Doc doc;
	if (!Load(pPath, "build-items", doc)) return false;
	const json * items = GetArray(doc.j, "items");
	if (items == nullptr) return false;
	for (const json & o : *items) {
		BuildItem it;
		const json * els = o.is_object() ? GetArray(o, "elements") : nullptr;
		bool ok = els != nullptr && els->size() == it.elements.size()
		       && GetString(o, "name", doc.cp, it.name) && GetInt(o, "skill_limit", it.skillLimit)
		       && GetInt(o, "sprite", it.sprite) && GetInt(o, "sprite_frame", it.spriteFrame)
		       && GetInt(o, "max_skill", it.maxSkill);
		for (std::size_t i = 0; ok && i < it.elements.size(); i++) {
			const json & e = (*els)[i];
			ok = e.is_object() && GetString(e, "name", doc.cp, it.elements[i].name)
			  && GetInt(e, "count", it.elements[i].count);
		}
		if (!ok) { out.clear(); return false; }
		out.push_back(std::move(it));
	}
	return true;
}

bool ReadMagicConfig(const char * pPath, std::vector<MagicCfg> & out)
{
	out.clear();
	Doc doc;
	if (!Load(pPath, "magic-config", doc)) return false;
	const json * arr = GetArray(doc.j, "magic");
	if (arr == nullptr) return false;
	for (const json & o : *arr) {
		MagicCfg m;
		const auto vis = o.is_object() ? o.find("visible") : o.end();
		if (!o.is_object() || vis == o.end() || !vis->is_boolean()
		    || !GetInt(o, "id", m.id) || !GetString(o, "name", doc.cp, m.name)
		    || !GetInt(o, "value1", m.value1) || !GetInt(o, "value2", m.value2)
		    || !GetInt(o, "value3", m.value3)) {
			out.clear();
			return false;
		}
		m.visible = vis->get<bool>();
		out.push_back(std::move(m));
	}
	return true;
}

bool ReadSkillConfig(const char * pPath, std::vector<SkillCfg> & out)
{
	out.clear();
	Doc doc;
	if (!Load(pPath, "skill-config", doc)) return false;
	const json * arr = GetArray(doc.j, "skills");
	if (arr == nullptr) return false;
	for (const json & o : *arr) {
		SkillCfg s;
		if (!o.is_object() || !GetInt(o, "id", s.id) || !GetString(o, "name", doc.cp, s.name)
		    || !GetInt(o, "useable", s.useable) || !GetInt(o, "use_method", s.useMethod)) {
			out.clear();
			return false;
		}
		out.push_back(std::move(s));
	}
	return true;
}

bool ReadLoginConfig(const char * pPath, LoginCfg & out)
{
	out = LoginCfg();
	Doc doc;
	if (!Load(pPath, "login-config", doc)) return false;
	if (doc.j.contains("log_server_address")) {
		if (!GetString(doc.j, "log_server_address", doc.cp, out.address)) return false;
		out.hasAddress = true;
	}
	if (doc.j.contains("log_server_port")) {
		if (!GetInt(doc.j, "log_server_port", out.port)) return false;
		out.hasPort = true;
	}
	return true;
}

namespace {

// Bytes in code page cp -> UTF-8, only when converting back gives the identical bytes.
bool FromClientString(const std::string & bytes, UINT cp, std::string & u8)
{
	u8.clear();
	if (bytes.empty()) return true;
	const int wlen = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
	if (wlen <= 0) return false;
	std::wstring w(wlen, L'\0');
	MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), &w[0], wlen);
	const int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), wlen, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return false;
	u8.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, w.data(), wlen, &u8[0], len, nullptr, nullptr);
	std::string back;
	return ToClientString(json(u8), cp, back) && back == bytes;
}

} // namespace

bool WriteNoticeContents(const char * pPath, const char * pText)
{
	const std::string full(pText);

	// The old path: fwrite to a "wt" file (each LF becomes CR LF) and fread from an "rt" file
	// (CR LF becomes LF, Ctrl-Z ends the file). The two translations cancel out, so the text
	// the old reader saw is the notice up to its first Ctrl-Z.
	std::string text = full.substr(0, full.find('\x1a'));
	unsigned long ulDiskSize = (unsigned long)full.size();
	for (char c : full) if (c == '\n') ulDiskSize++;

	const std::vector<std::string_view> tokens = SplitTokens(text, "\n");

	// The first encoding in which every line converts losslessly (latin-1 always does).
	static const struct { const char * pName; UINT cp; } kEncodings[] = {
		{ "ascii", 20127 }, { "cp949", 949 }, { "latin-1", 28591 } };
	for (const auto & enc : kEncodings) {
		json lines = json::array();
		bool ok = true;
		for (std::string_view t : tokens) {
			std::string u8;
			if (!FromClientString(std::string(t), enc.cp, u8)) { ok = false; break; }
			lines.push_back(u8);
		}
		if (!ok) continue;

		json doc;
		doc["format"] = "contents";
		doc["version"] = kFormatVersion;
		doc["encoding"] = enc.pName;
		doc["source"] = { { "file", "contents1000.txt" }, { "size", ulDiskSize }, { "origin", "server notice" } };
		doc["lines"] = lines;

		std::ofstream out(std::filesystem::u8path(pPath), std::ios::binary | std::ios::trunc);
		if (!out) return false;
		out << doc.dump(2) << "\n";
		return (bool)out;
	}
	return false;
}

unsigned long ReadNoticeSourceSize(const char * pPath)
{
	Doc doc;
	if (!Load(pPath, "contents", doc)) return 0;
	const auto src = doc.j.find("source");
	if (src == doc.j.end() || !src->is_object()) return 0;
	const auto size = src->find("size");
	if (size == src->end() || !size->is_number_unsigned()) return 0;
	return size->get<unsigned long>();
}

void CopyToBuffer(char * pDst, std::size_t size, const std::string & src)
{
	if (size == 0) return;
	std::memset(pDst, 0, size);
	std::memcpy(pDst, src.data(), (src.size() < size - 1) ? src.size() : size - 1);
}

} // namespace ClientData
