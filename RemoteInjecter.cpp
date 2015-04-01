/*!
	Description : This class is used to inject a dll or a piece of code
	              using "CreateRemoteThread" function.
	Author		: Ruining Song
	Date		: 2013.9.16
	Remark		:
			2013.10.29 >> 添加 InjectCode 函数，使用时需要注意：
						  1. 必须编译为 Release 版本，否则目标进程会崩溃
						  2. 所注入的线程中用到的所有的 api 地址和字符串必须由参数传入
						     （否则会发生重定位错误）
*/


#include "stdafx.h"

#include "RemoteInjecter.h"

#include <TlHelp32.h>

#include "DebugTools.h"

// 线程所占空间
#define THREAD_SIZE 1024

// /////////////////////////////////////////////////////////////////
// 注入 dll 
//
BOOL RemoteThreadInjecter::InjectDll(LPCWSTR wzTargetName, LPCWSTR wzDllName)
{
	HANDLE hProcess = GetProcessHandleByName(wzTargetName);

	if ( NULL == hProcess )
	{
		return FALSE;
	}

	return InjectDll(hProcess, wzDllName);
}

BOOL RemoteThreadInjecter::InjectDll(HANDLE hProcess, LPCWSTR wzDllName)
{
	// 提升进程权限
	if ( !EnableDebugPrivilege() )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectDll] EnableDebugPrivilege Failed.\r\n");
		return FALSE;
	}

	// 分配内存写入 dll 路径
	LPVOID lpBuff = VirtualAllocEx(
		hProcess,
		NULL,
		(lstrlenW(wzDllName)+1)*2,
		MEM_COMMIT,
		PAGE_READWRITE);
	DWORD dwWrite = 0;

	if ( !WriteProcessMemory(hProcess, lpBuff, (LPVOID) wzDllName, (lstrlenW(wzDllName)+1)*2, &dwWrite) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectDll] WriteProcessMemory Failed.\r\n");
		VirtualFreeEx(hProcess, lpBuff, (lstrlenW(wzDllName)+1)*2, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 取得 loadlibrary 地址
	LPTHREAD_START_ROUTINE lpFn = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32"), "LoadLibraryW");

	// 创建远程线程，线程执行函数为 loadlibrary
	DWORD dwThreadId = 0;
	HANDLE hThread = CreateRemoteThread(
		hProcess,
		NULL,
		0,
		lpFn,
		lpBuff,
		NULL,
		&dwThreadId);

	if ( NULL == hThread )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectDll] CreateRemoteThread Failed.\r\n");
		VirtualFreeEx(hProcess, lpBuff, (lstrlenW(wzDllName)+1)*2, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 等待 loadlibrary 执行完毕，释放资源
	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, lpBuff, (lstrlenW(wzDllName)+1)*2, MEM_DECOMMIT);
	CloseHandle(hProcess);
	CloseHandle(hThread);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// 注入代码 
//
BOOL RemoteThreadInjecter::InjectCode(LPWSTR wzTargetName, THREADFUNC lpThreadFunc, PVOID lpFuncPara, DWORD dwParaSize)
{
	HANDLE hProcess = GetProcessHandleByName(wzTargetName);

	if ( NULL == hProcess )
	{
		return FALSE;
	}

	// 提升进程权限
	if ( !EnableDebugPrivilege() )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] EnableDebugPrivilege Failed.\r\n");
		return FALSE;
	}
	
	// 在目标进程中分配要注入的代码所用的空间
	PVOID pRemoteThread = VirtualAllocEx(hProcess, NULL, THREAD_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if ( NULL == pRemoteThread )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Alloc Thread Space Failed.\r\n");
		CloseHandle(hProcess);
		return FALSE;
	}

	// 将线程代码写入
	if ( !WriteProcessMemory(hProcess, pRemoteThread, lpThreadFunc, THREAD_SIZE, 0) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Write Code To Process Failed.\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 分配参数所占空间
	LPVOID pRemotePara = VirtualAllocEx(hProcess, NULL, dwParaSize, MEM_COMMIT, PAGE_READWRITE);
	if( NULL == pRemotePara )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Alloc Parameter Space Failed.\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 写入参数
	if( !WriteProcessMemory(hProcess, pRemotePara, lpFuncPara, dwParaSize, 0) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Write Code Parameter To Process Failed\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		VirtualFreeEx(hProcess, pRemotePara, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 创建远程线程
	HANDLE hThread = NULL;
	DWORD dwThreadId = 0;
	hThread = CreateRemoteThread(hProcess, NULL, 0, (DWORD (WINAPI *)(LPVOID))pRemoteThread, pRemotePara, 0, &dwThreadId);
	if( NULL == hThread )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] CreateRemoteThread Failed.\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		VirtualFreeEx(hProcess, pRemotePara, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// 等待远程线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
	VirtualFreeEx(hProcess, pRemotePara, 0, MEM_RELEASE);
	CloseHandle(hProcess);
}

// /////////////////////////////////////////////////////////////////
// 由进程名得到进程句柄 
//
HANDLE RemoteThreadInjecter::GetProcessHandleByName(LPCWSTR wzProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if ( INVALID_HANDLE_VALUE == hSnapshot )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [GetProcessHandleByName] CreateToolhelp32Snapshot Failed.\r\n");
		return NULL;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if ( !Process32FirstW(hSnapshot, &pe32) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [GetProcessHandleByName] Process32FirstW Failed.\r\n");
		CloseHandle(hSnapshot);
		return NULL;
	}

	
	HANDLE hProcess = NULL;
	do 
	{
		if ( 0 == wcscmp(wzProcessName, pe32.szExeFile) )
		{
			hProcess = OpenProcess(
				PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
				FALSE, 
				pe32.th32ProcessID);

			if ( NULL == hProcess )
			{
				DebugTools::OutputDebugPrintfW(
					L"[RemoteInjecter] [GetProcessHandleByName] OpenProcess Failed.\r\n");
				CloseHandle(hSnapshot);
				return NULL;
			}

			break;
		}

	} while ( Process32NextW(hSnapshot, &pe32) );

	if ( NULL == hProcess )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [GetProcessHandleByName] Can't Find The Specified Process.\r\n");
	}

	CloseHandle(hSnapshot);

	return hProcess;
}

// /////////////////////////////////////////////////////////////////
// 提升进程权限 
//
BOOL RemoteThreadInjecter::EnableDebugPrivilege()
{
	HANDLE token;
	TOKEN_PRIVILEGES tp;

	// 打开进程令牌环
	if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token) )
	{
		return FALSE;
	}

	// 获得进程本地唯一ID
	LUID luid;
	if( !LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid) )
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;

	// 调整进程权限
	if( !AdjustTokenPrivileges(token,0,&tp,sizeof(TOKEN_PRIVILEGES),NULL,NULL) )
	{
		return FALSE;
	}

	CloseHandle(token);

	return TRUE;
}