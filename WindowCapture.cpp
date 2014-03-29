
#include "stdafx.h"
#include "FileTools.h"
#include "DebugTools.h"
#include "WindowCapture.h"
#include <atlimage.h>
#include <vfw.h>

#pragma comment(lib, "vfw32.lib")

#pragma warning(disable:4996)

bool WindowCapture::DoCapture( HWND hWnd, LPWSTR wzPicName )
{
	HDC hScreenDC = CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
	if ( NULL == hScreenDC ) return false;

	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);

	RECT rc = {0};

	if ( NULL == hWnd )
	{
		rc.right = GetSystemMetrics(SM_CXSCREEN);
		rc.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else
		::GetWindowRect(hWnd, &rc);

	HBITMAP hBitmap = ::CreateCompatibleBitmap(hScreenDC, rc.right-rc.left, rc.bottom-rc.top);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	::BitBlt(hMemDC, 0, 0, rc.right-rc.left, rc.bottom-rc.top, hScreenDC, rc.left, rc.top, SRCCOPY);
	
	CImage img;
	img.Attach(hBitmap);
	img.Save(wzPicName);

	::SelectObject(hMemDC, hOldBitmap);
	::ReleaseDC(hWnd, hScreenDC);
	::DeleteObject(hMemDC);
	::DeleteObject(hBitmap);

	return true;
}

void WindowCapture::BmpToAvi(LPWSTR wzAviName, LPWSTR wzPicPath, int frame)
{
	AVIFileInit();
	PAVIFILE pf = NULL;
	AVIFileOpen(&pf, wzAviName, OF_WRITE | OF_CREATE, NULL);
	AVISTREAMINFO strhdr = {0};
	strhdr.fccType = streamtypeVIDEO;
	strhdr.fccHandler = 0;
	strhdr.dwScale = 1;
	strhdr.dwRate = frame;

	PAVISTREAM ps = NULL;

	int fileNum = 1;
	WCHAR wzFileName[MAX_PATH] = {0};

	HRESULT hr = 0;
	
	do 
	{
		wsprintf(wzFileName, L"%s\\%d.bmp", wzPicPath, fileNum);
		BITMAPFILEHEADER bmpFileHdr = {0};
		BITMAPINFOHEADER bmpInfoHdr = {0};
		FILE* fp = _wfopen(wzFileName, L"rb");
		fseek(fp, 0, SEEK_SET);
		fread(&bmpFileHdr, sizeof(BITMAPFILEHEADER), 1, fp);
		fread(&bmpInfoHdr, sizeof(BITMAPINFOHEADER), 1, fp);
		bmpInfoHdr.biSizeImage = (bmpInfoHdr.biWidth*bmpInfoHdr.biBitCount+31)/32*4*bmpInfoHdr.biHeight;

		if ( 1 == fileNum )
		{
			strhdr.dwSuggestedBufferSize = bmpInfoHdr.biSizeImage;
			SetRect(&strhdr.rcFrame, 0, 0, bmpInfoHdr.biWidth, bmpInfoHdr.biHeight);
			AVIFileCreateStream(pf, &ps, &strhdr);
		}
		
		BYTE* buffer = new BYTE[bmpInfoHdr.biSizeImage];
		fread(buffer, 1, bmpInfoHdr.biSizeImage, fp);
		hr = AVIStreamSetFormat(ps, fileNum-1, &bmpInfoHdr, sizeof(bmpInfoHdr));
		hr = AVIStreamWrite(ps, fileNum-1, 1, buffer, bmpInfoHdr.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
		fclose(fp);
		delete buffer;

		DebugTools::OutputDebugPrintfW(L"[BmpToAvi] Processing... %d\r\n", fileNum);
		fileNum++;
		wsprintf(wzFileName, L"%s\\%d.bmp", wzPicPath, fileNum);

	} while ( FileTools::Exist(wzFileName) );

	AVIStreamClose(ps);
	AVIFileRelease(pf);
	AVIFileExit();
}

WindowCaptureThread::WindowCaptureThread(HWND hWindow, unsigned int frame, LPCWSTR wzPicPath)
	: m_hWindow(hWindow)
	, m_frame(frame)
	, m_wzPicPath(wzPicPath)
{
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, _T("VideoCapture"));
}

WindowCaptureThread::~WindowCaptureThread()
{
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hEvent);
}

void WindowCaptureThread::StartCapture()
{
	m_hThread = CreateThread(NULL, 1024, CaptureThread, (LPVOID)this, NULL, NULL);
}

void WindowCaptureThread::StopCapture()
{
	SetEvent(m_hEvent);
}

DWORD WINAPI WindowCaptureThread::CaptureThread(LPVOID lParam)
{
	WindowCaptureThread* owner = static_cast<WindowCaptureThread*>(lParam);
	int frame = 1;
	wchar_t wzPicName[MAX_PATH] = {0};

	while ( WAIT_OBJECT_0 != WaitForSingleObject(owner->m_hEvent, 0) )
	{
		Sleep(1000/owner->m_frame);
		wsprintf(wzPicName, L"%s\\%d.jpg", owner->m_wzPicPath, frame++);
		WindowCapture::DoCapture(owner->m_hWindow, wzPicName);
	}
	return 0;
}

