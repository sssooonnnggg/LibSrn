
//#include "stdafx.h"
#include "FileTools.h"
#include "DebugTools.h"
#include "WindowCapture.h"
#include <atlimage.h>
//#include <vfw.h>

//#pragma comment(lib, "vfw32.lib")

#pragma warning(disable:4996)

/*#include <d3d9.h>
#include <d3dx9.h>
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )

bool WindowCapture::DoCaptureUseD3D(HWND window, const wchar_t* output)
{
	LPDIRECT3D9 pD3D = NULL;
	D3DDISPLAYMODE ddm; 
	D3DPRESENT_PARAMETERS d3dpp;

	IDirect3DDevice9 * pd3dDevice;
	IDirect3DSurface9 * pSurface;

	ZeroMemory(&d3dpp,sizeof(D3DPRESENT_PARAMETERS)); 
	ZeroMemory(&d3dpp, sizeof(d3dpp) );


	if( NULL == ( pD3D = Direct3DCreate9(D3D_SDK_VERSION) ) ) 
		return false;

	if( FAILED( pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm) ) ) 
		return false;

	d3dpp.Windowed = TRUE; 
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; 
	d3dpp.BackBufferFormat = ddm.Format; 
	d3dpp.BackBufferHeight = ddm.Height; 
	d3dpp.BackBufferWidth = ddm.Width; 
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE; 
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; 
	d3dpp.hDeviceWindow = GetDesktopWindow(); 
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT; 
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;


	if( FAILED( pD3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		GetDesktopWindow(),
		D3DCREATE_SOFTWARE_VERTEXPROCESSING ,
		&d3dpp,
		&pd3dDevice) ) ) 
		return false;

	if( FAILED( pd3dDevice->CreateOffscreenPlainSurface(
		ddm.Width, 
		ddm.Height, 
		D3DFMT_A8R8G8B8, 
		D3DPOOL_SCRATCH, 
		&pSurface,
		NULL) ) ) 
		return false;

	if ( FAILED( pd3dDevice->CreateOffscreenPlainSurface(
		GetSystemMetrics(SM_CXSCREEN), 
		GetSystemMetrics(SM_CYSCREEN), 
		D3DFMT_A8R8G8B8, 
		D3DPOOL_SCRATCH,
		&pSurface, 
		NULL) ) )
		return false;

	if ( FAILED ( pd3dDevice->GetFrontBufferData(0, pSurface) ) )
		return false;

	WINDOWINFO info = {0};

	if ( window )
		GetWindowInfo(window, &info);

	if ( FAILED ( D3DXSaveSurfaceToFile(output, D3DXIFF_PNG, pSurface, NULL, window ? &info.rcWindow : NULL ) ) )
		return false;

	//D3DXSaveSurfaceToFile(L"Desktop.jpg",D3DXIFF_JPG,pSurface,NULL,NULL);//保存为 jpg格式
	//D3DXSaveSurfaceToFile(L"Desktop.bmp",D3DXIFF_BMP,pSurface,NULL,NULL);//保存为 bmp格式
	pSurface->Release(); 
	pd3dDevice->Release();
	pD3D->Release();

	return true;

}*/

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

/*void WindowCapture::BmpToAvi(LPWSTR wzAviName, LPWSTR wzPicPath, int frame)
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
}*/

WindowCaptureThread::WindowCaptureThread(HWND hWindow, double frame, LPCWSTR wzPicPath)
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
	ResetEvent(m_hEvent);
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
		wsprintf(wzPicName, L"%s\\Capture_%.3d.jpg", owner->m_wzPicPath.c_str(), frame++);
		WindowCapture::DoCapture(owner->m_hWindow, wzPicName);
		Sleep(1000.0/owner->m_frame);
	}
	return 0;
}

