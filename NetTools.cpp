/*!
	Decription : Useful Network Utils.
	Author     : Ruining Song
	Date       : 2013.9.16
	Remark     :
*/

#include "stdafx.h"
#include "NetTools.h"
#include "DebugTools.h"

class NetToolsInternal
{

public:
	// ////////////////////////////////////////////////////////////////////////////////
	// @private StateChange 
	//
	static BOOL StateChange(DWORD NewState, DWORD SelectedItem,HDEVINFO hDevInfo)
	{
		SP_PROPCHANGE_PARAMS PropChangeParams = {sizeof(SP_CLASSINSTALL_HEADER)};
		SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};

		if ( !SetupDiEnumDeviceInfo(hDevInfo,SelectedItem,&DeviceInfoData) )
		{
			return FALSE;
		}

		PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		PropChangeParams.Scope = DICS_FLAG_GLOBAL;
		PropChangeParams.StateChange = NewState; 

		if ( !SetupDiSetClassInstallParams(
			hDevInfo,
			&DeviceInfoData,
			(SP_CLASSINSTALL_HEADER *)&PropChangeParams,
			sizeof(PropChangeParams)) )
		{
			return FALSE;
		}

		if ( !SetupDiCallClassInstaller(
			DIF_PROPERTYCHANGE,
			hDevInfo,
			&DeviceInfoData) )
		{
			return TRUE;
		}

		return TRUE;
	}

	// ////////////////////////////////////////////////////////////////////////////////
	// @private IsClassNet 
	//
	static BOOL IsClassNet( GUID * ClassGuid )
	{
#define MAX_NUM  50

		HKEY hKeyClass;
		LONG lRet;
		WCHAR ClassType[MAX_NUM];
		WCHAR NetClass[MAX_NUM] = L"Net";
		DWORD dwLength = MAX_NUM,dwType = REG_SZ;

		if (hKeyClass = SetupDiOpenClassRegKey(ClassGuid,KEY_READ))
		{
			lRet = RegQueryValueExW(
				hKeyClass, 
				L"Class", 
				NULL, 
				&dwType, 
				LPBYTE(ClassType), 
				&dwLength);

			RegCloseKey(hKeyClass);

			if ( lRet != ERROR_SUCCESS )
			{
				return FALSE;
			}

			if ( !wcscmp(ClassType,NetClass) )
			{
				return TRUE;
			}
		}                                 

		return FALSE;
	}

};

// ////////////////////////////////////////////////////////////////////////////////
// …Ë÷√Õ¯ø®◊¥Ã¨
// 1 - ∆Ù”√
// 0 - Ω˚”√
//
BOOL NetTools::SetMacState(BOOL bState)
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(
		NULL,
		NULL,
		0,
		DIGCF_PRESENT | DIGCF_ALLCLASSES
		);

	if ( INVALID_HANDLE_VALUE == hDevInfo )
	{
		DebugTools::OutputDebugPrintf(L"[NetTools] [SetMacState] SetupDiGetClassDevs Failed.\r\n");
		return FALSE;
	}

	SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};

	for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
	{

		if ( NetToolsInternal::IsClassNet(&DeviceInfoData.ClassGuid) )
		{
			DWORD dwStatus = bState ? DICS_ENABLE : DICS_DISABLE;

			if ( !NetToolsInternal::StateChange(dwStatus, i, hDevInfo) )
			{
				DebugTools::OutputDebugPrintf(L"[NetTools] [SetMacState] StateChange Failed.\r\n");
			}
			else
			{
				return TRUE;
			}
		}
	}

	return TRUE;
}