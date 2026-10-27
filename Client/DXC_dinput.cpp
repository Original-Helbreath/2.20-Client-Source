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

BOOL DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst, bool bWindowed)
{
 HRESULT hr;
 DIMOUSESTATE dims;
 POINT Point;

	m_hWnd      = hWnd;
	m_bWindowed = bWindowed;
	GetCursorPos(&Point);
	m_sX     = (short)(Point.x);
	m_sY     = (short)(Point.y);
	m_sLastX = m_sX;
	m_sLastY = m_sY;

	hr = LegacyDirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, NULL );
    if (hr != DI_OK) return FALSE;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, NULL );
	if (hr != DI_OK) return FALSE;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return FALSE;
	// RENDER-016: windowed, the mouse is shared with Windows (the position comes from the system cursor).
	hr = m_pMouse->SetCooperativeLevel( hWnd, (m_bWindowed ? DISCL_NONEXCLUSIVE : DISCL_EXCLUSIVE) | DISCL_FOREGROUND);
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
		if (m_bWindowed == false) return;
		// RENDER-016: windowed, the game also draws while another window has focus (DirectInput then has
		// no state): report the cursor position with no buttons and no wheel, so callers never read garbage.
		ZeroMemory(&dims, sizeof(dims));
	}
	if (m_bWindowed) {
		_UpdateWindowedMouse(pX, pY, pZ, pLB, pRB);
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

// RENDER-016: windowed mouse. The position is the system cursor in client coordinates (so it follows the
// window when it moves); buttons and wheel still come from DirectInput (dims, read by the caller), and
// count only while the cursor is over the client area, so clicks on the title bar do not reach the game.
// When the game moved m_sX/m_sY itself (it centres the cursor on some screens), the system cursor follows.
void DXC_dinput::_UpdateWindowedMouse(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	POINT ptCursor;
	if ((m_sX != m_sLastX || m_sY != m_sLastY) && GetForegroundWindow() == m_hWnd) {
		ptCursor.x = m_sX;
		ptCursor.y = m_sY;
		ClientToScreen(m_hWnd, &ptCursor);
		SetCursorPos(ptCursor.x, ptCursor.y);
	}

	GetCursorPos(&ptCursor);
	ScreenToClient(m_hWnd, &ptCursor);
	const bool bInside = (ptCursor.x >= 0) && (ptCursor.y >= 0) && (ptCursor.x <= 639) && (ptCursor.y <= 479);
	if (ptCursor.x < 0) ptCursor.x = 0;
	if (ptCursor.y < 0) ptCursor.y = 0;
	if (ptCursor.x > 639) ptCursor.x = 639;
	if (ptCursor.y > 479) ptCursor.y = 479;

	m_sX = m_sLastX = (short)ptCursor.x;
	m_sY = m_sLastY = (short)ptCursor.y;
	if (bInside && (short)dims.lZ != 0) m_sZ = (short)dims.lZ;
	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = bInside ? (char)dims.rgbButtons[0] : 0;
	*pRB = bInside ? (char)dims.rgbButtons[1] : 0;
}