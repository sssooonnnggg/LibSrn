/*!
	Description : Used To Add/Extract File To/From PE Resource Section.
	Authro		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
			�����ļ���Ƕ�뵽ͬһ����Դ ID ��

			��Դ���ͣ�RESOURCE_TYPE
			��ԴID	��RESOURCE_ID
			��Դ�ṹ��| �ļ��� | �ļ���С | �ļ����� | �ļ���2 | �ļ���С2 | �ļ����� |...
*/

//#include "stdafx.h"
#include "ResourceMgr.h"
#include "FileTools.h"
#include "DebugTools.h"

const DWORD ResourceMgr::RESOURCE_ID = 0x12F8023;
const LPCWSTR ResourceMgr::RESOURCE_TYPE = L"SRN_RESOURCE";

// ////////////////////////////////////////////////////////////////////////////////
// �򵥵��ڴ�����װ 
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
// �ļ�ͷ�� 
//
typedef struct _FILE_HEADER
{
	WCHAR wzFileName[MAX_PATH];
	DWORD dwFileSize;
}FILE_HEADER, *PFILE_HEADER;

// ////////////////////////////////////////////////////////////////////////////////
// ���캯�� 
//
ResourceMgr::ResourceMgr(LPCWSTR lpwzExePath)
{
	// ���� 10M �ռ�
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
// �������� 
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
// ����ļ� 
//
BOOL ResourceMgr::AddFile(LPCWSTR lpwzFileName)
{
	if ( !FileTools::Exist(lpwzFileName) )
	{
		return FALSE;
	}

	//// ��ȡ��Դ��������
	//ExtractToBuffer();
	
	//
	// ����Դĩβ����ļ�
	//
	LPBYTE lpTail = m_lpBuffer->buffer + m_lpBuffer->curlen;
	DWORD dwFileSize = FileTools::GetFileSize(lpwzFileName);

	// ������ļ�ͷ
	FILE_HEADER header;
	header.dwFileSize = dwFileSize;
	FileTools::GetFileNameWithoutPath(lpwzFileName, header.wzFileName);
	memcpy(lpTail, &header, sizeof(FILE_HEADER));
	m_lpBuffer->curlen += sizeof(FILE_HEADER);
	lpTail += sizeof(FILE_HEADER);

	// ������ļ�����
	FileTools::ReadFileToMem(lpwzFileName, lpTail);
	m_lpBuffer->curlen += dwFileSize;

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// ���������ļ���ִ��
//
BOOL ResourceMgr::Packet()
{
	return AddBufferToResource();
}

// ////////////////////////////////////////////////////////////////////////////////
// �ͷ��ļ� 
//
BOOL ResourceMgr::Extract(LPCWSTR lpwzFileName)
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// ѭ�������ļ��Ƿ�����Դ��
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
// �ͷ�ȫ���ļ� 
//
BOOL ResourceMgr::ExtractAll()
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// ѭ�������ļ��Ƿ�����Դ��
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
// �ͷ������ļ�
//
BOOL ResourceMgr::ExtractAll(LPCWSTR lpwzPath)
{
	if ( !ExtractToBuffer() )
	{
		return FALSE;
	}

	DWORD dwHeaderLen = sizeof(FILE_HEADER);
	PFILE_HEADER lpHeader = NULL;

	// ѭ�������ļ��Ƿ�����Դ��
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
// @private �ͷ���Դ�� m_lpBuffer 
//
BOOL ResourceMgr::ExtractToBuffer()
{

	// ��ȡ PE ���
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
	
	// ������Դ������ҵ��˲Ŷ����� Buffer
	HRSRC hRes = FindResourceExW(hExe, RESOURCE_TYPE, MAKEINTRESOURCEW(RESOURCE_ID), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	if ( NULL == hRes )
	{
		FreeLibrary(hExe);
		Sleep(100);
		return FALSE;
	}
	
	// ������Դ
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