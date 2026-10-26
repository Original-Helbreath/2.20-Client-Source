// StrTok.h: interface for the CStrTok class.
//
// Replacement for StrTok.lib / the client's old in-tree StrTok.cpp. Same constructor and pGet()
// as the original CStrTok, same tokens returned, but the token is held in a growable buffer
// instead of a fixed char[256] / char[1024], so long tokens are no longer truncated or overrun.
// Plain C++98 + STL so it still builds with Visual C++ 6 (see README.md).
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRTOK_H__A9554BE2_A96B_11D2_B143_00001C7030A6__INCLUDED_)
#define AFX_STRTOK_H__A9554BE2_A96B_11D2_B143_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <vector>

class CStrTok
{
public:
	// pData and pSeps are not copied: both must stay valid (and unchanged) while pGet() is used,
	// exactly as with the original class.
	CStrTok(char * pData, char * pSeps);
	virtual ~CStrTok();

	// Returns the next token, or NULL when the input is used up. The returned pointer refers to a
	// buffer owned by this object and is overwritten by the next pGet() call.
	char * pGet();

	char * m_pData, * m_pSeps;
	int    m_iDataLength, m_iCurLoc;

private:
	bool _bIsSeperator(char cData, char cNextData) const;
	char _cNextData() const;

	std::vector<char> m_cToken;   // current token plus its terminating NUL
};

#endif // !defined(AFX_STRTOK_H__A9554BE2_A96B_11D2_B143_00001C7030A6__INCLUDED_)
