/*!
	Description : Capture window
	Author		: Ruining Song
	Date		: 2014/2/24
*/

#ifndef WINDOWCAPTURE
#define WINDOWCAPTURE

#include <Windows.h>

class WindowCapture
{
public:
	/*!
	\param
		The window to capture. If NULL, capture full screen.
	\param
		The picture name of capture.
	*/
	static bool DoCapture(HWND hWnd, LPWSTR wzPicName);

	static void BmpToAvi(LPWSTR wzAviName, LPWSTR wzPicPath, int frame = 30);
};

class WindowCaptureThread
{
public:

	/*!
	\param
		Window's handle
	\param
		Frame rate
	\param
		Path to save capture
	*/
	WindowCaptureThread(HWND hWindow, unsigned int frame, LPCWSTR wzPicPath);
	~WindowCaptureThread();

	void StartCapture();
	void StopCapture();

private:
	static DWORD WINAPI CaptureThread(LPVOID lParam);

private:
	HWND m_hWindow;
	HANDLE m_hThread;
	HANDLE m_hEvent;
	unsigned int m_frame;
	LPCWSTR m_wzPicPath;
};

#endif // WINDOWCAPTURE