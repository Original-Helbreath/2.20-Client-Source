// DXC_dinput.h: interface for the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
#define AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define INITGUID
#include <windows.h>
#include "dinput.h"

class DXC_dinput  
{
public:
	DXC_dinput();
	virtual ~DXC_dinput();
	void UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB);
	void SetAcquire(BOOL bFlag);
	BOOL bInit(HWND hWnd, HINSTANCE hInst, bool bWindowed = false);

	DIMOUSESTATE dims;
	IDirectInput *           m_pDI;
	IDirectInputDevice *     m_pMouse;
	short m_sX, m_sY, m_sZ;

private:
	// RENDER-016: windowed mode reads the system cursor instead of accumulating relative motion.
	void _UpdateWindowedMouse(short * pX, short * pY, short * pZ, char * pLB, char * pRB);
	HWND  m_hWnd = NULL;
	bool  m_bWindowed = false;
	short m_sLastX = 0, m_sLastY = 0;
};

#endif // !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
