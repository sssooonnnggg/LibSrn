#include "ScreenRecord.h"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "mfuuid.lib")

EXTERN_C const GUID CLSID_PushSourceDesktop = 
{0x4ea6930a, 0x2c8a, 0x4ae6, {0xa5,   0x61,   0x56,   0xe4,   0xb5, 0x4, 0x44, 0x37}};

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, const GUID& clsid, LPCWSTR wszName, IBaseFilter **ppF)
{
	if ( !pGraph || !ppF ) return E_POINTER;

	*ppF = 0;
	IBaseFilter *pF = 0;
	HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void**>(&pF));

	if ( SUCCEEDED(hr) )
	{
		hr = pGraph->AddFilter(pF, wszName);

		if (SUCCEEDED(hr))
			*ppF = pF;
		else
			pF->Release();
	}

	return hr;
}

HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);

	if ( FAILED(hr) )
		return hr;

	while ( pEnum->Next(1, &pPin, NULL) == S_OK )
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);

		if ( ThisPinDir == PinDir )
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);

			if ( SUCCEEDED(hr) ) 
			{
				pTmp->Release();
			}
			else
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}

		pPin->Release();
	}

	pEnum->Release();

	return E_FAIL;
}


HRESULT ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest)
{
	if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
		return E_POINTER;

	IPin *pIn = 0;
	HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);

	if ( FAILED(hr) )
		return hr;

	hr = pGraph->Connect(pOut, pIn);
	pIn->Release();
	return hr;
}

HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
	if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
		return E_POINTER;

	IPin *pOut = 0;
	HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);

	if ( FAILED(hr) )
		return hr;

	hr = ConnectFilters(pGraph, pOut, pDest);
	pOut->Release();
	return hr;
}

void RegisterFilter(LPCWSTR wzDll) 
{
	HMODULE hDll = LoadLibraryW(wzDll);

	if ( hDll )
	{
		typedef void (WINAPI *REG)();
		REG regFunc = (REG)GetProcAddress(hDll, "DllRegisterServer");

		if ( regFunc )
			regFunc();

		FreeLibrary(hDll);
	}
}

template<typename T> void SafeRelease(T* p)
{
	if ( NULL != p )
		p->Release();
}

ScreenRecord::ScreenRecord(LPCWSTR wzAviName, LPCWSTR wzFilterPath)
	: m_wzAviName(wzAviName)
	, m_pGraph(NULL)
	, m_pDeskTop(NULL)
	, m_pAviMux(NULL)
	, m_pFileWrite(NULL)
	, m_pDes(NULL)
	, m_pControl(NULL)
{
	CoInitialize(NULL);
	RegisterFilter(wzFilterPath);
	InitConnection();
}

ScreenRecord::~ScreenRecord()
{
	SafeRelease(m_pGraph);
	SafeRelease(m_pDeskTop);
	SafeRelease(m_pAviMux);
	SafeRelease(m_pFileWrite);
	SafeRelease(m_pDes);
	SafeRelease(m_pControl);
	CoUninitialize();
}

void ScreenRecord::InitConnection()
{
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&m_pGraph);

	AddFilterByCLSID(m_pGraph, CLSID_PushSourceDesktop, L"PushSourceDestktop Filter", &m_pDeskTop);
	AddFilterByCLSID(m_pGraph, CLSID_AviDest, L"AviMux Filter", &m_pAviMux);
	//AddFilterByCLSID(m_pGraph, CLSID_WMAsfWriter, L"AviMux Filter", &m_pAviMux);
	AddFilterByCLSID(m_pGraph, CLSID_FileWriter, L"FileWrite Filter", &m_pFileWrite);

	ConnectFilters(m_pGraph, m_pDeskTop, m_pAviMux);
	ConnectFilters(m_pGraph, m_pAviMux, m_pFileWrite);

	IFileSinkFilter* pdes = NULL;
	m_pFileWrite->QueryInterface(IID_IFileSinkFilter,(void**)&m_pDes);
	m_pDes->SetFileName(m_wzAviName, NULL);

	m_pGraph->QueryInterface(IID_IMediaControl, (void **)&m_pControl);
}

void ScreenRecord::StartRecord()
{
	m_pControl->Run();
}

void ScreenRecord::StopRecord()
{
	m_pControl->Stop();
}