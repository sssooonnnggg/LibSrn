
#include "stdafx.h"
#include "WindowWrapper.h"

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

#ifdef _DEBUG
	ShowWindow(m_hWnd, SW_SHOW);
#endif

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

