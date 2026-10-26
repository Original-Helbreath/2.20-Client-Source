// StrSplit.h: stateless string splitting for typed chat commands and the command line.
//
// SplitTokens() returns the same tokens, in the same order, as the removed CStrTok class did:
//  - a character is a separator if it is '\r' followed by '\n', or if it occurs in seps
//    (data ends at its first NUL, as with the C string the class was given);
//  - a token is the run of non-separators at the current position, and it is returned as soon
//    as a separator is met, even when it is empty (so leading separators give one "" token);
//  - after a token, the run of separators is skipped, but the skip stops once the position
//    reaches the last character, so two or more trailing separators give one more "" token;
//  - the list ends when the input is used up with nothing collected.
// The returned views point into data, which must outlive them.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <string_view>
#include <vector>

inline std::vector<std::string_view> SplitTokens(std::string_view data, std::string_view seps)
{
	const std::size_t n = data.size();
	auto isSep = [&](std::size_t p) {
		const char c = data[p];
		if (c == '\0') return true;
		if (c == '\r' && p + 1 < n && data[p + 1] == '\n') return true;
		return seps.find(c) != std::string_view::npos;
	};

	std::vector<std::string_view> tokens;
	std::size_t pos = 0;
	while (true) {
		const std::size_t start = pos;
		while (pos < n && !isSep(pos)) pos++;
		if (pos < n) {
			const std::size_t end = pos;
			bool done = false;
			while (!done) {
				if (isSep(pos)) pos++;
				else done = true;
				if (pos + 1 >= n) done = true;   // pos >= n - 1
			}
			tokens.push_back(data.substr(start, end - start));
			continue;
		}
		if (pos == start) break;               // nothing collected: end of the list
		tokens.push_back(data.substr(start, pos - start));
	}
	return tokens;
}

// Convenience: token i, or an empty optional-like (nullptr data) view when there is none.
inline std::string_view TokenAt(const std::vector<std::string_view>& tokens, std::size_t i)
{
	return i < tokens.size() ? tokens[i] : std::string_view();
}
