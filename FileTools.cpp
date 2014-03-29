/*!
	Description : Useful File Utils.
	Author		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
*/

#include "stdafx.h"
#include "FileTools.h"
#include "DebugTools.h"

#include <string>

// ////////////////////////////////////////////////////////////////////////////////
// �ж��ļ��Ƿ���� 
//
BOOL FileTools::Exist(LPCWSTR lpwzFilePath)
{

	DWORD dwResult = GetFileAttributesW(lpwzFilePath);

	if ( INVALID_FILE_ATTRIBUTES == dwResult
		&& ERROR_FILE_NOT_FOUND == GetLastError() )
	{
		return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// ��ȡ�ļ�����Ŀ¼ 
//
VOID FileTools::GetFileDir(IN LPCWSTR lpwzFile, OUT LPWSTR lpwzFilePath)
{
	std::wstring file(lpwzFile);
	std::wstring::size_type index = 0;

	index = file.find_last_of(L'\\');

	if ( -1 != index )
	{
		wcscpy_s(lpwzFilePath, MAX_PATH, file.substr(0, index).c_str());
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// ��ȡ������������·��
//
VOID FileTools::GetExePath(OUT LPWSTR lpwzExePath)
{
	GetModuleFileNameW(NULL, lpwzExePath, MAX_PATH);
	LPWSTR wzEnd = wcsrchr(lpwzExePath, '\\');
	*wzEnd = L'\0';
}

// ////////////////////////////////////////////////////////////////////////////////
// ��ȡ�ļ���С
//
DWORD FileTools::GetFileSize(LPCWSTR lpwzFile)
{
	if ( !Exist(lpwzFile) )
	{
		return -1;
	}

	HANDLE hFile = CreateFileW(
		lpwzFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	
	DWORD dwDump = 0;
	DWORD dwFileSize = ::GetFileSize(hFile, &dwDump);
	CloseHandle(hFile);

	return dwFileSize;
}

// ////////////////////////////////////////////////////////////////////////////////
// ��ȡ�ļ����ڴ� 
//
BOOL FileTools::ReadFileToMem(LPCWSTR lpwzFile, LPBYTE lpBuffer)
{
	HANDLE hFile = CreateFileW(
		lpwzFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	
	if ( INVALID_HANDLE_VALUE == hFile )
	{
		DebugTools::OutputDebugPrintf(L"[FileTools] [ReadFileToMem] CreateFile Failed.[%s]\r\n", lpwzFile);
		return FALSE;
	}

	DWORD dwSize = GetFileSize(lpwzFile);
	DWORD dwRead = 0;

	if ( !ReadFile(hFile, lpBuffer, dwSize, &dwRead, NULL) )
	{
		DebugTools::OutputDebugPrintf(L"[FileTools] [ReadFileToMem] Read File Failed.[%d]\r\n", GetLastError());
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// WriteFileFromMem 
//
BOOL FileTools::WriteFileFromMem(LPCWSTR lpwzFile, LPBYTE lpBuffer, DWORD dwLen)
{
	HANDLE hFile = CreateFileW(
		lpwzFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if ( INVALID_HANDLE_VALUE == hFile )
	{
		DebugTools::OutputDebugPrintf(L"[FileTools] [WriteFileFromMem] CreateFile Failed..[%s]\r\n", lpwzFile);
		return FALSE;
	}

	DWORD dwWrite = 0;

	if ( !WriteFile(hFile, lpBuffer, dwLen, &dwWrite, NULL) )
	{
		DebugTools::OutputDebugPrintf(L"[FileTools] [WriteFileFromMem] Write File Failed.[%d]\r\n", GetLastError());
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// ��·����ȡ�ļ��� 
//
VOID FileTools::GetFileNameWithoutPath(LPCWSTR lpwzPath, LPWSTR lpwzName)
{
	std::wstring file(lpwzPath);
	std::wstring::size_type index = 0;

	index = file.find_last_of(L'\\');

	if ( -1 != index )
	{
		wcscpy(lpwzName, file.substr(index+1).c_str());
	}
	else
	{
		wcscpy(lpwzName, lpwzPath);
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// �������Ŀ¼
//
VOID FileTools::CreateDirectorys(LPCWSTR lpwzDir)
{
	std::wstring path = lpwzDir;

	int pos = path.find(L'\\');

	while ( std::string::npos != pos )
	{
		std::wstring subStr = path.substr(0, pos);
		CreateDirectoryW(subStr.c_str(), NULL);
		pos = path.find(L'\\', pos+1);
	}

	CreateDirectoryW(lpwzDir, NULL);
}