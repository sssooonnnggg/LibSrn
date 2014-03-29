/*!
	Description : Simple Window Wrapper
	Author		: Ruining Song
	Date		: 2014/2/17
*/

#ifndef WINDOWWRAPPER_H
#define WINDOWWRAPPER_H

#include <Windows.h>

class WindowWrapper
{
public:
	WindowWrapper(HINSTANCE hInst) : m_hInstance(hInst), m_hWnd(NULL) {}

	typedef LRESULT (WINAPI *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	void RegisterClass(WNDPROC lpProc);
	virtual void InitInstance();
	virtual void MessageLoop();

	HWND Hwnd() { return m_hWnd; }

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
};

#endif // WINDOWWRAPPER_H