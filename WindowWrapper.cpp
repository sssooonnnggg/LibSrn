
//#include "stdafx.h"
#include "WindowWrapper.h"
#include "DebugTools.h"

#include <tchar.h>

void WindowWrapper::RegisterClass(WNDPROC lpProc)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= lpProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= _T("WindowWrapper");
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

void WindowWrapper::InitInstance()
{
	m_hWnd = CreateWindow(_T("WIndowWrapper"), _T("WindowWrapper"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInstance, NULL);

	int id = GetLastError();

	DebugTools::OutputDebugPrintfW(L"[WindowsWrapper] CreateWindow : %d\r\n", id);

	if ( m_showWnd )
		ShowWindow(m_hWnd, SW_SHOW);

	UpdateWindow(m_hWnd);
}

void WindowWrapper::MessageLoop()
{
	MSG msg;
	while ( GetMessage(&msg, NULL, 0, 0) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

WindowWrapperThread::WindowWrapperThread() 
	: m_thread(NULL) 
{
	m_initEvent = CreateEvent(NULL, TRUE, FALSE, L"InitInstance");
	m_loopEvent = CreateEvent(NULL, TRUE, FALSE, L"MessageLoop");
}

WindowWrapperThread::~WindowWrapperThread()
{	
	if ( m_thread )
		CloseHandle(m_thread);

	CloseHandle(m_initEvent);
	CloseHandle(m_loopEvent);
}

void WindowWrapperThread::Init()
{
	m_thread = CreateThread(NULL, 1024, WindowThread, (LPVOID)this, NULL, NULL);
}

void WindowWrapperThread::InitInstance()
{
	SetEvent(m_initEvent);
}

void WindowWrapperThread::MessageLoop()
{
	SetEvent(m_loopEvent);
}

DWORD WindowWrapperThread::WindowThread(LPVOID param)
{
	WindowWrapperThread* owner = static_cast<WindowWrapperThread*>(param);
	WaitForSingleObject(owner->m_initEvent, INFINITE);
	owner->m_window.InitInstance();
	WaitForSingleObject(owner->m_loopEvent, INFINITE);
	owner->m_window.MessageLoop();

	return 0;
}