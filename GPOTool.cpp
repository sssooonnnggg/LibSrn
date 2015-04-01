/*!
	Description : Add Group Policy log off/start scripts.
	Author		: Ruining Song
	Date		: 2013.9.25
	Remark		:
*/

//#include "stdafx.h"
#include "GPOTool.h"
#include "DebugTools.h"

// ////////////////////////////////////////////////////////////////////////////////
// �ڲ�ʵ�� 
//
class GpoToolImpl
{
public:

	// ////////////////////////////////////////////////////////////////////////////////
	// ���캯�� 
	//
	GpoToolImpl()
	{
		InitPath();
	}
	
	// ////////////////////////////////////////////////////////////////////////////////
	// �޸� gpt.ini 
	//
	BOOL ModifyGpt()
	{
		BOOL bRet = FALSE;

		do 
		{

			if ( !WritePrivateProfileStringW(
				L"General", 
				L"gPCMachineExtensionNames", 
				L"[{35378EAC-683F-11D2-A89A-00C04FBBCFA2}{0F6B957D-509E-11D1-A7CC-0000F87571E3}]", 
				m_wzGptPath) )
			{
				break;
			}

			if ( !WritePrivateProfileStringW(
				L"General", 
				L"Version", 
				L"131073", 
				m_wzGptPath) )
			{
				break;
			}

			if ( !WritePrivateProfileStringW(
				L"General", 
				L"gPCUserExtensionNames", 
				L"[{42B5FAAE-6536-11D2-AE5A-0000F87571E3}{40B66650-4972-11D1-A7CA-0000F87571E3}]", 
				m_wzGptPath) )
			{
				break;
			}

			if ( !WritePrivateProfileStringW(
				L"General", 
				L"Options", 
				L"0", 
				m_wzGptPath) )
			{
				break;
			}

			bRet = TRUE;

		} while (FALSE);

		return bRet;
	}

	// ////////////////////////////////////////////////////////////////////////////////
	// �޸� script.ini 
	//
	BOOL ModifyScript(LPCWSTR wzSection, LPCWSTR wzScriptPath)
	{
		BOOL bRet = FALSE;
		
		do 
		{
			if ( !WritePrivateProfileStringW(wzSection, L"0CmdLine", wzScriptPath, m_wzScriptPath) )
			{
				break;
			}
			
			if ( !WritePrivateProfileStringW(wzSection, L"0Parameters", L" ", m_wzScriptPath) )
			{
				break;
			}

			bRet = TRUE;

		} while (FALSE);
		
		return bRet;
	}

private:

	// ////////////////////////////////////////////////////////////////////////////////
	// ��ʼ��·�� 
	//
	inline VOID InitPath()
	{
		memset(m_wzGptPath, 0, MAX_PATH*2);
		memset(m_wzScriptPath, 0, MAX_PATH*2);

		WCHAR wzSysDir[MAX_PATH] = {0};
		GetSystemDirectoryW(wzSysDir, MAX_PATH);
		wsprintfW(m_wzGptPath, L"%s\\GroupPolicy\\gpt.ini", wzSysDir);
		DebugTools::OutputDebugPrintfW(L"[GpoToolImpl] Gpt Path : %s\r\n", m_wzGptPath);

		// ���Ŀ¼�����ڣ��������Ŀ¼
		WCHAR wzGroupPolicy[MAX_PATH] = {0};
		wsprintfW(wzGroupPolicy, L"%s\\GroupPolicy", wzSysDir);
		CreateDirectoryW(wzGroupPolicy, NULL);
		WCHAR wzUser[MAX_PATH] = {0};
		wsprintfW(wzUser, L"%s\\GroupPolicy\\User", wzSysDir);
		CreateDirectoryW(wzUser, NULL);
		WCHAR wzScripts[MAX_PATH] = {0};
		wsprintfW(wzScripts, L"%s\\GroupPolicy\\User\\Scripts", wzSysDir);
		CreateDirectoryW(wzScripts, NULL);
		WCHAR wzScriptsLogOn[MAX_PATH] = {0};
		WCHAR wzScriptsLogOff[MAX_PATH] = {0};
		wsprintfW(wzScriptsLogOn, L"%s\\GroupPolicy\\User\\Scripts\\Logon", wzSysDir);
		wsprintfW(wzScriptsLogOff, L"%s\\GroupPolicy\\User\\Scripts\\Logoff", wzSysDir);
		CreateDirectoryW(wzScriptsLogOn, NULL);
		CreateDirectoryW(wzScriptsLogOff, NULL);

		wsprintfW(m_wzScriptPath, L"%s\\GroupPolicy\\User\\Scripts\\scripts.ini", wzSysDir);
		DebugTools::OutputDebugPrintfW(L"[GpoToolImpl] Script Path : %s\r\n", m_wzScriptPath);
	}

private:
	WCHAR m_wzGptPath[MAX_PATH];				// gpt.ini ·��
	WCHAR m_wzScriptPath[MAX_PATH];				// scripts.ini ·��
};

// ////////////////////////////////////////////////////////////////////////////////
// ���캯�� 
//
GpoTool::GpoTool()
	: m_pGpoToolImpl(new GpoToolImpl)
{

}

// ////////////////////////////////////////////////////////////////////////////////
// �������� 
//
GpoTool::~GpoTool()
{
	if ( m_pGpoToolImpl )
	{
		delete m_pGpoToolImpl;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// ���ע���ű� 
//
BOOL GpoTool::AddLogOffScript(LPCWSTR wzScriptPath)
{
	if ( !m_pGpoToolImpl->ModifyGpt() )
	{
		DebugTools::OutputDebugPrintfW(L"[GpoTool] [AddLogOffScript] ModifyGpt Failed.\r\n");
		return FALSE;
	}

	return m_pGpoToolImpl->ModifyScript(L"Logoff", wzScriptPath);
}

// ////////////////////////////////////////////////////////////////////////////////
// ��������ű� 
//
BOOL GpoTool::AddStartScript(LPCWSTR wzScriptPath)
{
	if ( !m_pGpoToolImpl->ModifyGpt() )
	{
		DebugTools::OutputDebugPrintfW(L"[GpoTool] [AddLogOffScript] ModifyGpt Failed.\r\n");
		return FALSE;
	}

	return m_pGpoToolImpl->ModifyScript(L"Logon", wzScriptPath);
}