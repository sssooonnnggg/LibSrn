/*!
	Decription : Useful Registry Utils.
	Author     : Ruining Song
	Date       : 2013.9.17
	Remark     :
			2013-9-27 >> ����� RegGetPrivileg RegGetAllAccess
*/

#include "stdafx.h"
#include "RegTools.h"
#include "DebugTools.h"

#include <AccCtrl.h>
#include <Aclapi.h>

#pragma warning(disable : 4996)

// ////////////////////////////////////////////////////////////////////////////////
// �޸�ע����ַ�����ֵ
// ������ 1 - ע���������� HKEY_LOCAL_MACHINE
//        2 - Ҫ�޸ĵ����λ�ã��� SOFTWARE\\Microsoft\\Windows NT
//        3 - Ҫ�޸ĵ�����
//        4 - Ҫ�޸ĵ�ֵ
//
BOOL RegTools::RegSetString(HKEY hkRoot, LPCWSTR wzLocation, LPCWSTR wzName, LPCWSTR wzValue)
{
	HKEY hKey;

	LRESULT lResult = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		wzLocation,
		0,
		KEY_ALL_ACCESS,
		&hKey);

	if ( ERROR_SUCCESS != lResult )
	{
		RegCloseKey(hKey);
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegSetString] RegOpenKeyEx Failed. [%d]\r\n", 
			lResult);
		return FALSE;
	}

	WCHAR wzValueBuffer[MAX_PATH] = {0};
	wcscpy(wzValueBuffer, wzValue);

	lResult = RegSetValueExW(
		hKey,
		wzName,
		NULL,
		REG_SZ,
		(PBYTE)wzValueBuffer,
		(lstrlenW(wzValueBuffer)+1)*2 );

	if ( ERROR_SUCCESS != lResult )
	{
		RegCloseKey(hKey);
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegSetString] RegSetValueEx Failed. [%d]\r\n", 
			lResult);
		return FALSE;
	}

	RegCloseKey(hKey);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// �������̲���ע����Ȩ��
//
BOOL RegTools::RegGetPrivilege()
{
	HANDLE hToken = NULL;
	LUID rLuid;
	LUID bLuid;

	if ( !OpenProcessToken(
		GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
		&hToken) )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetPrivilege] OpenProcessToken Failed. [%d]\r\n",
			GetLastError());
		return FALSE;
	}

	int offset = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[2]);
	PTOKEN_PRIVILEGES tkp = (PTOKEN_PRIVILEGES) malloc(offset);
	tkp->PrivilegeCount = 2;
	tkp->Privileges[0];	

	if ( !LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &bLuid) )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetPrivilege] LookupPrivilegeValueW Failed. [%d]\r\n",
			GetLastError());
		CloseHandle(hToken);
		return FALSE;
	}

	tkp->Privileges[0].Luid = bLuid;
	tkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if ( !LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &rLuid) )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetPrivilege] LookupPrivilegeValueW Failed. [%d]\r\n",
			GetLastError());
		CloseHandle(hToken);
		return FALSE;
	}

	tkp->Privileges[1].Luid = rLuid;
	tkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

	if ( !AdjustTokenPrivileges(hToken, FALSE, tkp, 0, NULL, 0) )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetPrivilege] AdjustTokenPrivileges Failed. [%d]\r\n",
			GetLastError());
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////////
// ������ǰ�˻�����ע����Ȩ��
// ������ 1 - SubKey ·�� ���� MACHINE\\STempSAM\\SAM
//
BOOL RegTools::RegGetAllAccess(LPCWSTR lpwzSubKeyName)
{
	WCHAR szSamName[MAX_PATH] = {0};
	PACL pOldDacl=NULL;
	PACL pNewDacl=NULL;
	DWORD dRet;
	EXPLICIT_ACCESSW eia;
	PSECURITY_DESCRIPTOR pSID=NULL;

	wcscpy(szSamName, lpwzSubKeyName);

	dRet = GetNamedSecurityInfoW(szSamName, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDacl, NULL, &pSID);

	if( ERROR_SUCCESS != dRet )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetAllAccess] GetNamedSecurityInfo Failed. [%d]\r\n",
			dRet);
		return FALSE;
	}

	ZeroMemory(&eia, sizeof(EXPLICIT_ACCESS));
	BuildExplicitAccessWithNameW(&eia, L"Administrators", KEY_ALL_ACCESS, SET_ACCESS, SUB_CONTAINERS_AND_OBJECTS_INHERIT);

	dRet = SetEntriesInAclW(1, &eia, pOldDacl, &pNewDacl);

	if( ERROR_SUCCESS != dRet )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetAllAccess] SetEntriesInAcl Failed. [%d]\r\n",
			dRet);
		return FALSE;
	}

	WCHAR lpwzNameCopy[MAX_PATH] = {0};
	lstrcpyW(lpwzNameCopy, lpwzSubKeyName);
	dRet = SetNamedSecurityInfoW(lpwzNameCopy, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL);

	if( ERROR_SUCCESS != dRet )
	{
		DebugTools::OutputDebugPrintf(
			L"[RegTools] [RegGetAllAccess] SetNamedSecurityInfo Failed. [%d]\r\n",
			dRet);
		return FALSE;
	}

	if( pNewDacl )
	{
		LocalFree(pNewDacl);
	}

	if( pSID )
	{
		LocalFree(pSID);
	}

	return TRUE;
}