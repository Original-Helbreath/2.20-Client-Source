// StrTok.cpp: implementation of the CStrTok class.
//
// Tokenising rules, kept from the original (StrTok.lib == game server StrTok.cpp):
//  - a character is a separator if it is NUL, if it is '\r' followed by '\n', or if it occurs
//    in pSeps;
//  - a token is the run of non-separators starting at the current position;
//  - after a token's terminating separator, all following separators are skipped, but the skip
//    stops once the position reaches the last character of the input;
//  - pGet() returns the token as soon as a separator is met, even when the token is empty
//    (so leading separators, or two or more trailing ones, produce one "" token), and returns
//    NULL only when the end of input is reached with no characters collected.
//
//////////////////////////////////////////////////////////////////////

#include "StrTok.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStrTok::CStrTok(char * pData, char * pSeps)
	: m_pData(pData), m_pSeps(pSeps), m_iDataLength((int)strlen(pData)), m_iCurLoc(0)
{
	// Same capacity as the original char[1024]: the pointer pGet() returns keeps the same address
	// for every token up to that size; only a longer token reallocates.
	m_cToken.reserve(1024);
}

CStrTok::~CStrTok()
{
}

//////////////////////////////////////////////////////////////////////
// Tokenising
//////////////////////////////////////////////////////////////////////

// The character after the current one, or NUL at the last character.
char CStrTok::_cNextData() const
{
	return (m_iCurLoc <= m_iDataLength - 2) ? m_pData[m_iCurLoc + 1] : '\0';
}

bool CStrTok::_bIsSeperator(char cData, char cNextData) const
{
	if (cData == '\0') return true;
	if ((cData == '\r') && (cNextData == '\n')) return true;
	return strchr(m_pSeps, cData) != NULL;   // strchr never matches NUL here: cData != '\0'
}

char * CStrTok::pGet()
{
	m_cToken.clear();

	while (m_iCurLoc < m_iDataLength) {
		if (!_bIsSeperator(m_pData[m_iCurLoc], _cNextData())) {
			m_cToken.push_back(m_pData[m_iCurLoc]);
			m_iCurLoc++;
			continue;
		}

		// Separator: skip the run of separators, stopping at the last character, then return the
		// token collected so far (possibly empty).
		bool bDone = false;
		while (!bDone) {
			if (_bIsSeperator(m_pData[m_iCurLoc], _cNextData())) m_iCurLoc++;
			else bDone = true;

			if (m_iCurLoc >= m_iDataLength - 1) bDone = true;
		}

		m_cToken.push_back('\0');
		return &m_cToken[0];
	}

	if (m_cToken.empty()) return NULL;

	m_cToken.push_back('\0');
	return &m_cToken[0];
}
