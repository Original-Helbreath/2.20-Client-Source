// winmain.h

BOOL InitApplication( HINSTANCE hInstance);
BOOL InitInstance( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK WndProc( HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam );
void EventLoop();
void OnDestroy();
void Initialize(char * pCmdLine);
void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD_PTR dwUSer, DWORD_PTR dw1, DWORD_PTR dw2);
MMRESULT _StartTimer(DWORD dwTime);

void _StopTimer(MMRESULT timerid);
//void PutLogFileList(char * cStr);
//void __WriteChatLogFile(char * cStr);
void CPSocketConnet() ;  // v2.03
LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
void GoHomepage();