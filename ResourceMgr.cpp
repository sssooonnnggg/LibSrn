/*!
	Description : Used To Add/Extract File To/From PE Resource Section.
	Authro		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
			所有文件被嵌入到同一个资源 ID 下

			资源类型：RESOURCE_TYPE
			资源ID	：RESOURCE_ID
			资源结构：| 文件名 | 文件大小 | 文件内容 | 文件名2 | 文件大小2 | 文件内容 |...
*/

//#include "stdafx.h"
#include "ResourceMgr.h"
#include "FileTools.h"
#include "DebugTools.h"

const DWORD ResourceMgr::RESOURCE_ID = 0x12F8023;
const LPCWSTR ResourceMgr::RESOURCE_TYPE = L"SRN_RESOURCE";

// ////////////////////////////////////////////////////////////////////////////////
// 简单的内存分配封装 
//
class Buffer
{
public:
	Buffer()
		: buffer(NULL)
		, curlen(0)
		, total(0)
	{
	}
	
public:
	LPBYTE buffer;
	DWORD curlen;
	DWORD total;
};

// ////////////////////////////////////////////////////////////////////////////////
// 文件头部 
//
typedef struct _FILE_HEADER
{
	WCHAR wzFileName[MAX_PATH];
	DWORD dwFileSize;
}FILE_HEADER, *PFILE_HEADER;

// ////////////////////////////////////////////////////////////////////////////////
// 构造函数 
//
ResourceMgr::ResourceMgr(LPCWSTR lpwzExePath)
{
	// 分配 10M 空间
	m_lpBuffer = new Buffer;
	m_lpBuffer->buffer = new BYTE[1024*1024*10];
	m_lpBuffer->total = 1024*1024*10;

	ZeroMemory(m_wzExePath, MAX_PATH);

	if ( NULL != lpwzExePath )
	{
		wcscpy_s(m_wzExePath, lpwzExePath);
	}
};

// ////////////////////////////////////////////////////////////////////////////////
// 析构函数 
//
ResourceMgr::~ResourceMgr()
{
	if ( m_lpBuffer->buffer )
	{
		delete [] m_lpBuffer->buffer;
	}

	delete m_lpBuffer;
}

// ////////////////////////////////////////////////////////////////////////////////
// 添加文件 
//
BOOL ResourceMgr::AddFile(LPCWSTR lpwzFileName)
{
	if ( !FileTools::Exist(lpwzFileName) )
	{
		return FALSE;
	}

	//// 提取资源至缓冲区
	//ExtractToBuffer();
	
	//
	// 在资源末尾添加文件
	//
	LPBYTE lpTail = m_lpBuffer->buffer + m_lpBuffer->curlen;
	DWORD dwFileSize = FileTools::GetFileSize(lpwzFileName);

	// 先添加文件头
	FILE_HEADER header;
	header.dwFileSize = dwFileSize;
	FileTools::GetFileNameWithoutPath(lpwzFileName, header.wzFileName);
	memcpy(lpTail, &header, sizeof(FILE_HEADER));
	m_lpBuffer->curlen += sizeof(FILE_HEADER);
	lpTail += sizeof(FILE_HEADER);

	// 再添加文件内容
	FileTools::ReadFileToMem(lpwzFileName, lpTail);
	m_lpBuffer->curlen += dwFileSize;

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// 打包，添加文件后执行
//
BOOL ResourceMgr::Packet()
{
	return AddBufferToResource();
}

// ////////////////////////////////////////////////////////////////////////////////
// 释放文件 
//
BOOL ResourceMgr::Extract(LPCWSTR lpwzFileName)
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// 循环查找文件是否在资源中
	DWORD i = 0;
	while ( i < m_lpBuffer->curlen )
	{
		lpHeader = (PFILE_HEADER)(m_lpBuffer->buffer+i);

		if ( 0 == wcscmp(lpwzFileName, lpHeader->wzFileName) )
		{
			break;
		}
		else
		{
			i += sizeof(FILE_HEADER) + lpHeader->dwFileSize;
		}
	}

	if ( i < m_lpBuffer->curlen )
	{
		WCHAR wzFullPath[MAX_PATH] = {0};
		WCHAR wzExePath[MAX_PATH] = {0};
		FileTools::GetExePath(wzExePath);
		wsprintfW(wzFullPath, L"%s\\%s", wzExePath, lpHeader->wzFileName);
		//FileTools::CreateDirectorys(wzFullPath);
		FileTools::WriteFileFromMem(wzFullPath, m_lpBuffer->buffer+i+sizeof(FILE_HEADER), lpHeader->dwFileSize);
		return TRUE;
	}
	else
	{
		DebugTools::OutputDebugPrintfW(L"[ResourceMgr] [Extract] Can Not Find File. [%s]\r\n", lpwzFileName);
		return FALSE;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// 释放全部文件 
//
BOOL ResourceMgr::ExtractAll()
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// 循环查找文件是否在资源中
	for (DWORD i = 0; i < m_lpBuffer->curlen; )
	{
		lpHeader = (PFILE_HEADER)(m_lpBuffer->buffer+i);
		WCHAR wzFullPath[MAX_PATH] = {0};
		WCHAR wzExePath[MAX_PATH] = {0};
		FileTools::GetExePath(wzExePath);
		wsprintfW(wzFullPath, L"%s\\%s", wzExePath, lpHeader->wzFileName);
		//FileTools::CreateDirectorys(wzFullPath);
		FileTools::WriteFileFromMem(wzFullPath, m_lpBuffer->buffer+i+sizeof(FILE_HEADER), lpHeader->dwFileSize);
		DebugTools::OutputDebugPrintfW(L"[ResourceMgr] [ExtractAll] Exacting File : %s\r\n", lpHeader->wzFileName);
		i += sizeof(FILE_HEADER) + lpHeader->dwFileSize;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// 释放所有文件
//
BOOL ResourceMgr::ExtractAll(LPCWSTR lpwzPath)
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// 循环查找文件是否在资源中
	for (DWORD i = 0; i < m_lpBuffer->curlen; )
	{
		lpHeader = (PFILE_HEADER)(m_lpBuffer->buffer+i);
		WCHAR wzFullPath[MAX_PATH] = {0};
		wsprintfW(wzFullPath, L"%s\\%s", lpwzPath, lpHeader->wzFileName);
		//FileTools::CreateDirectorys(wzFullPath);
		FileTools::WriteFileFromMem(wzFullPath, m_lpBuffer->buffer+i+sizeof(FILE_HEADER), lpHeader->dwFileSize);
		DebugTools::OutputDebugPrintfW(L"[ResourceMgr] [ExtractAll] Exacting File : %s\r\n", lpHeader->wzFileName);
		i += sizeof(FILE_HEADER) + lpHeader->dwFileSize;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// @private 释放资源至 m_lpBuffer 
//
BOOL ResourceMgr::ExtractToBuffer()
{

	// 获取 PE 句柄
	HMODULE hExe = NULL;

	if ( 0 != wcslen(m_wzExePath) )
	{
		hExe = LoadLibraryW(m_wzExePath);
	}
	else
	{
		hExe = GetModuleHandle(NULL);
	}

	if ( NULL == hExe )
	{
		DebugTools::OutputDebugPrintfW(L"[ResourceMgr] [ExtractToBuffer] Open Exe Failed.[%s]\r\n", m_wzExePath);
		return FALSE;
	}
	
	// 查找资源，如果找到了才读入至 Buffer
	HRSRC hRes = FindResourceExW(hExe, RESOURCE_TYPE, MAKEINTRESOURCEW(RESOURCE_ID), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	if ( NULL == hRes )
	{
		FreeLibrary(hExe);
		Sleep(100);
		return FALSE;
	}
	
	// 加载资源
	HANDLE hResLoad = LoadResource(hExe, hRes);
	if ( NULL == hRes )
	{
		DebugTools::OutputDebugPrintfW(L"[ResourceMgr] [ExtractToBuffer] LoadResource Failed.\r\n");
		EndUpdateResourceW(hResLoad, FALSE);
		FreeLibrary(hExe);
		return FALSE;
	}
	
	DWORD dwSize = SizeofResource(hExe, hRes);
	PBYTE lpRscBuffer = (PBYTE)LockResource(hResLoad);

	memcpy(m_lpBuffer->buffer, lpRscBuffer, dwSize);
	m_lpBuffer->curlen = dwSize;

	EndUpdateResourceW(hResLoad, FALSE);

	FreeLibrary(hExe);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// AddBufferToResource
//
BOOL ResourceMgr::AddBufferToResource()
{
	HANDLE hRsc = BeginUpdateResourceW(m_wzExePath, FALSE);

	if ( NULL == hRsc )
	{
		DebugTools::OutputDebugPrintfW(
			L"[ResourceMgr] [AddBufferToResource] BeginUpdateResourceW Failed. [%d] \r\n", GetLastError());
		return FALSE;
	}

	if ( !UpdateResourceW(
		hRsc, 
		RESOURCE_TYPE, 
		MAKEINTRESOURCEW(RESOURCE_ID), 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), 
		m_lpBuffer->buffer, 
		m_lpBuffer->curlen) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[ResourceMgr] [AddBufferToResource] UpdateResourceW Failed. [%d] \r\n", GetLastError());
		return FALSE;
	}

	if ( !EndUpdateResourceW(hRsc, FALSE) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[ResourceMgr] [AddBufferToResource] EndUpdateResourceW Failed. [%d] \r\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}