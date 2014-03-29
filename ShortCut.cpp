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
// 构造函数 
//
ShortCut::ShortCut()
{
	CoInitialize(NULL);
	memset(m_wzExePath, 0, MAX_PATH*2);
	memset(m_wzLinkPath, 0, MAX_PATH*2);
	m_wHotKey = 0;
}

// ////////////////////////////////////////////////////////////////////////////////
// 析构函数 
//
ShortCut::~ShortCut()
{
	CoUninitialize();
}

// ////////////////////////////////////////////////////////////////////////////////
// 设置 exe 路径 
//
VOID ShortCut::SetExePath(LPCWSTR lpwzExePath)
{
	wcscpy(m_wzExePath, lpwzExePath);
}

// ////////////////////////////////////////////////////////////////////////////////
// 设置快捷方式路径 
//
VOID ShortCut::SetLinkPath(LPCWSTR lpwzLinkPath)
{
	wcscpy(m_wzLinkPath, lpwzLinkPath);
}

// ////////////////////////////////////////////////////////////////////////////////
// 设置快捷键 
//
VOID ShortCut::SetHotKey(WORD wHotKey)
{
	m_wHotKey = wHotKey;
}

// ////////////////////////////////////////////////////////////////////////////////
// 创建快捷方式 
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
		
		// 获取 IPersistFile 接口
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