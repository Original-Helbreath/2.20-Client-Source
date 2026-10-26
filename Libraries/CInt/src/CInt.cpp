// CInt.cpp: implementation of the CInt class (recreated from Cint.lib, member .\Release\Cint.obj).
//
// CInt keeps an int in its own 4-byte VirtualAlloc block, XORed with a random key, so the plain
// value never sits in the owning object. The block is made read-only after every Set().
// Each function below was written from the object code and does the same calls in the same
// order; see ../README.md and ../decompile/Cint.c.
//
//////////////////////////////////////////////////////////////////////

#include "Cint.h"
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// ??0CInt@@QAE@XZ
CInt::CInt()
{
	// No NULL check in the original either.
	m_pValue    = (int *)VirtualAlloc(NULL, sizeof(int), MEM_COMMIT, PAGE_READWRITE);
	*m_pValue   = 0;
	m_iKeyValue = rand();
	*m_pValue  ^= m_iKeyValue;   // stores 0 ^ key; the page stays writable until the first Set()
}

// ??1CInt@@QAE@XZ
CInt::~CInt()
{
	// MEM_DECOMMIT, not MEM_RELEASE: the address range stays reserved (as in the original).
	VirtualFree(m_pValue, sizeof(int), MEM_DECOMMIT);
}

//////////////////////////////////////////////////////////////////////
// Access
//////////////////////////////////////////////////////////////////////

// ?Get@CInt@@QAEHXZ
int CInt::Get()
{
	return *m_pValue ^ m_iKeyValue;
}

// ?Set@CInt@@QAEXABH@Z
void CInt::Set(const int & iValue)
{
	// If the page cannot be made writable the call does nothing: value and key are unchanged.
	if (VirtualProtect(m_pValue, sizeof(int), PAGE_READWRITE, &old) == FALSE) return;

	m_iKeyValue = rand();          // a fresh key on every write
	*m_pValue   = iValue ^ m_iKeyValue;

	VirtualProtect(m_pValue, sizeof(int), PAGE_READONLY, &old);
}

// ??4CInt@@QAEAAV0@ABH@Z
CInt & CInt::operator=(const int & iValue)
{
	Set(iValue);
	return *this;
}

// ??BCInt@@QAEHXZ (compiled as a jump to Get)
CInt::operator int()
{
	return Get();
}

// Cint.lib also defines the two operators below (??8CInt@@QAE_NABH@Z and ??9CInt@@QAE_NABH@Z),
// but Cint.h has their declarations commented out, so they are kept here commented out too.
// Restore both the declarations and these bodies together if they are ever needed.
//
// bool CInt::operator==(const int & iValue)
// {
// 	return Get() == iValue;
// }
//
// bool CInt::operator!=(const int & iValue)
// {
// 	return Get() != iValue;
// }
