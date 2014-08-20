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
	WindowWrapper(HINSTANCE hInst = NULL) : m_hInstance(hInst), m_hWnd(NULL) {}

	typedef LRESULT (WINAPI *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	void RegisterClass(WNDPROC lpProc);
	virtual void InitInstance();
	virtual void MessageLoop();

	HWND Hwnd() { return m_hWnd; }

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
};

/*!
\remark
	This class creats a window in a separate thread.
*/
class WindowWrapperThread
{
public:
	WindowWrapperThread();

	~WindowWrapperThread();

	void RegisterClass(WNDPROC lpProc)
	{
		m_window.RegisterClass(lpProc);
	}

	HWND Hwnd() { return m_window.Hwnd(); }

	void Init();
	void InitInstance();
	void MessageLoop();

private:
	static DWORD WINAPI WindowThread(LPVOID param);

private:
	HANDLE m_thread;
	HANDLE m_initEvent;
	HANDLE m_loopEvent;

	WindowWrapper m_window;
};

#endif // WINDOWWRAPPER_H