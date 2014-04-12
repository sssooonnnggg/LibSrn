#include "DbgHelper.h"

#include <Windows.h>
#include <Dbghelp.h>
#include <iostream>
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>

#pragma comment(lib, "DbgHelp.lib")

void CreateMiniDump(EXCEPTION_POINTERS* e)
{
	wchar_t filePath[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, filePath, MAX_PATH);
	*wcsrchr(filePath, L'\\') = L'\0';

	wchar_t fileName[MAX_PATH] = {0};
	SYSTEMTIME time = {0};
	GetSystemTime(&time);
	wsprintfW(fileName, L"%s\\CrashDump__%4d%02d%02d_%02d%02d%02d",
		filePath,
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

	HANDLE file = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if( file == INVALID_HANDLE_VALUE )
		return;

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		file,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithFullMemory),
		e ? &exceptionInfo : NULL,
		NULL,
		NULL);

	if(file)
	{
		CloseHandle(file);
		file = NULL;
	}

	return; 
}

LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS* e)
{
	CreateMiniDump(e);
	return EXCEPTION_CONTINUE_SEARCH;
}

void DbgHelper::InstallDbgHelper()
{
	SetUnhandledExceptionFilter(ExceptionHandler);
}