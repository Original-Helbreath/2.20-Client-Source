// DXC_dinput.cpp: implementation of the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_pDI    = NULL;
	m_pMouse = NULL;
	m_sX     = 0;
	m_sY     = 0;
	m_sZ     = 0;
}

DXC_dinput::~DXC_dinput()
{
	if (m_pMouse != NULL) {
		m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = NULL;
	}
	if (m_pDI != NULL) {
		m_pDI->Release();
        m_pDI = NULL;
	}
}

// SETUP-014: the Windows SDK no longer ships dinput.lib (DirectInput 7 import library), only dinput8.lib.
// DirectInputCreateA is still exported by the system dinput.dll, so it is resolved at run time.
static HRESULT LegacyDirectInputCreate(HINSTANCE hInst, DWORD dwVersion, LPDIRECTINPUT * ppDI, LPUNKNOWN pUnkOuter)
{
	typedef HRESULT (WINAPI * PFN_DirectInputCreateA)(HINSTANCE, DWORD, LPDIRECTINPUTA *, LPUNKNOWN);
	static HMODULE hDInput = LoadLibraryExA("dinput.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDInput == NULL) return DIERR_GENERIC;
	PFN_DirectInputCreateA pfnCreate = reinterpret_cast<PFN_DirectInputCreateA>(GetProcAddress(hDInput, "DirectInputCreateA"));
	if (pfnCreate == NULL) return DIERR_GENERIC;
	return pfnCreate(hInst, dwVersion, ppDI, pUnkOuter);
}

BOOL DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
 HRESULT hr;
 DIMOUSESTATE dims;	
 POINT Point;

	GetCursorPos(&Point);
	m_sX     = (short)(Point.x);
	m_sY     = (short)(Point.y); 

	hr = LegacyDirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, NULL );
    if (hr != DI_OK) return FALSE;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, NULL );
	if (hr != DI_OK) return FALSE;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return FALSE;
	hr = m_pMouse->SetCooperativeLevel( hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if (hr != DI_OK) return FALSE;

//	m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		//return TRUE;
	}

	return TRUE;
}


void DXC_dinput::SetAcquire(BOOL bFlag)
{
 DIMOUSESTATE dims;

	if (m_pMouse == NULL) return;
	if (bFlag == TRUE) {
		m_pMouse->Acquire();
		m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
	}
	else m_pMouse->Unacquire();
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		return;
	}
	m_sX += (short)dims.lX;
	m_sY += (short)dims.lY;
	if( (short)dims.lZ != 0 )m_sZ = (short)dims.lZ;
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > 639) m_sX = 639;
	if (m_sY > 479) m_sY = 479;
	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = (char)dims.rgbButtons[0];
	*pRB = (char)dims.rgbButtons[1];
}