/*!
	Description : Used To Create a Short Cut.
	Author		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
*/

#include "stdafx.h"
#include "ShortCut.h"
#include "FileTools.h"
#include "DebugTools.h"

#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

// ////////////////////////////////////////////////////////////////////////////////
// ���캯�� 
//
ShortCut::ShortCut()
{
	CoInitialize(NULL);
	memset(m_wzExePath, 0, MAX_PATH*2);
	memset(m_wzLinkPath, 0, MAX_PATH*2);
	m_wHotKey = 0;
}

// ////////////////////////////////////////////////////////////////////////////////
// �������� 
//
ShortCut::~ShortCut()
{
	CoUninitialize();
}

// ////////////////////////////////////////////////////////////////////////////////
// ���� exe ·�� 
//
VOID ShortCut::SetExePath(LPCWSTR lpwzExePath)
{
	wcscpy(m_wzExePath, lpwzExePath);
}

// ////////////////////////////////////////////////////////////////////////////////
// ���ÿ�ݷ�ʽ·�� 
//
VOID ShortCut::SetLinkPath(LPCWSTR lpwzLinkPath)
{
	wcscpy(m_wzLinkPath, lpwzLinkPath);
}

// ////////////////////////////////////////////////////////////////////////////////
// ���ÿ�ݼ� 
//
VOID ShortCut::SetHotKey(WORD wHotKey)
{
	m_wHotKey = wHotKey;
}

// ////////////////////////////////////////////////////////////////////////////////
// ������ݷ�ʽ 
//
BOOL ShortCut::CreateShortCut()
{
	BOOL bRet = FALSE;
	HRESULT hr;
	IShellLinkW* pLink = NULL;
	IPersistFile* ppf = NULL;

	do 
	{
		if ( !FileTools::Exist(m_wzExePath) )
		{
			break;
		}

		hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pLink);
		if ( FAILED(hr) )
		{
			break;
		}
		
		// ��ȡ IPersistFile �ӿ�
		hr = pLink->QueryInterface(IID_IPersistFile, (void**)&ppf);
		if ( FAILED(hr) )
		{
			break;
		}
		
		pLink->SetPath(m_wzExePath);
		pLink->SetHotkey(m_wHotKey);
		ppf->Save(m_wzLinkPath, TRUE);
		bRet = TRUE;

	} while (FALSE);

	if ( ppf )
	{
		ppf->Release();
	}

	if ( pLink )
	{
		pLink->Release();
	}

	return bRet;
}