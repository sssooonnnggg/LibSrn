/*!
	Description : This class is used to inject a dll or a piece of code
	              using "CreateRemoteThread" function.
	Author		: Ruining Song
	Date		: 2013.9.16
	Remark		:
			2013.10.29 >> ��� InjectCode ������ʹ��ʱ��Ҫע�⣺
						  1. �������Ϊ Release �汾������Ŀ����̻����
						  2. ��ע����߳����õ������е� api ��ַ���ַ��������ɲ�������
						     ������ᷢ���ض�λ����
*/


#include "stdafx.h"

#include "RemoteInjecter.h"

#include <TlHelp32.h>

#include "DebugTools.h"

// �߳���ռ�ռ�
#define THREAD_SIZE 1024

// /////////////////////////////////////////////////////////////////
// ע�� dll 
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
	// ��������Ȩ��
	if ( !EnableDebugPrivilege() )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectDll] EnableDebugPrivilege Failed.\r\n");
		return FALSE;
	}

	// �����ڴ�д�� dll ·��
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

	// ȡ�� loadlibrary ��ַ
	LPTHREAD_START_ROUTINE lpFn = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32"), "LoadLibraryW");

	// ����Զ���̣߳��߳�ִ�к���Ϊ loadlibrary
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

	// �ȴ� loadlibrary ִ����ϣ��ͷ���Դ
	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, lpBuff, (lstrlenW(wzDllName)+1)*2, MEM_DECOMMIT);
	CloseHandle(hProcess);
	CloseHandle(hThread);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// ע����� 
//
BOOL RemoteThreadInjecter::InjectCode(LPWSTR wzTargetName, THREADFUNC lpThreadFunc, PVOID lpFuncPara, DWORD dwParaSize)
{
	HANDLE hProcess = GetProcessHandleByName(wzTargetName);

	if ( NULL == hProcess )
	{
		return FALSE;
	}

	// ��������Ȩ��
	if ( !EnableDebugPrivilege() )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] EnableDebugPrivilege Failed.\r\n");
		return FALSE;
	}
	
	// ��Ŀ������з���Ҫע��Ĵ������õĿռ�
	PVOID pRemoteThread = VirtualAllocEx(hProcess, NULL, THREAD_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if ( NULL == pRemoteThread )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Alloc Thread Space Failed.\r\n");
		CloseHandle(hProcess);
		return FALSE;
	}

	// ���̴߳���д��
	if ( !WriteProcessMemory(hProcess, pRemoteThread, lpThreadFunc, THREAD_SIZE, 0) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Write Code To Process Failed.\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// ���������ռ�ռ�
	LPVOID pRemotePara = VirtualAllocEx(hProcess, NULL, dwParaSize, MEM_COMMIT, PAGE_READWRITE);
	if( NULL == pRemotePara )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Alloc Parameter Space Failed.\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// д�����
	if( !WriteProcessMemory(hProcess, pRemotePara, lpFuncPara, dwParaSize, 0) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[RemoteInjecter] [InjectCode] Write Code Parameter To Process Failed\r\n");
		VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
		VirtualFreeEx(hProcess, pRemotePara, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	// ����Զ���߳�
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

	// �ȴ�Զ���߳̽���
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	VirtualFreeEx(hProcess, pRemoteThread, 0, MEM_RELEASE);
	VirtualFreeEx(hProcess, pRemotePara, 0, MEM_RELEASE);
	CloseHandle(hProcess);
}

// /////////////////////////////////////////////////////////////////
// �ɽ������õ����̾�� 
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
// ��������Ȩ�� 
//
BOOL RemoteThreadInjecter::EnableDebugPrivilege()
{
	HANDLE token;
	TOKEN_PRIVILEGES tp;

	// �򿪽������ƻ�
	if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token) )
	{
		return FALSE;
	}

	// ��ý��̱���ΨһID
	LUID luid;
	if( !LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid) )
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;

	// ��������Ȩ��
	if( !AdjustTokenPrivileges(token,0,&tp,sizeof(TOKEN_PRIVILEGES),NULL,NULL) )
	{
		return FALSE;
	}

	CloseHandle(token);

	return TRUE;
}