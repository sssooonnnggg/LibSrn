/*!
	Description : Used To Create a Short Cut.
	Author		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
*/

#ifndef SHORTCUT_H
#define SHORTCUT_H

#include <Windows.h>

class ShortCut
{
public:

	// ���캯��
	ShortCut();

	// ����
	~ShortCut();
	
	// ���� exe �ļ�·��
	VOID SetExePath(LPCWSTR lpwzExePath);

	// ���ÿ�ݷ�ʽ·��
	VOID SetLinkPath(LPCWSTR lpwzLinkPath);

	// ���ÿ�ݼ�
	// eg�� SetHotKey(MAKEWORD(VK_F12, HOTKEYF_CONTROL))
	// ���ÿ�ݼ�Ϊ Ctrl + F7
	VOID SetHotKey(WORD wHotKey);

	// ������ݷ�ʽ
	BOOL CreateShortCut();

private:
	WCHAR m_wzExePath[MAX_PATH];
	WCHAR m_wzLinkPath[MAX_PATH];
	WORD m_wHotKey; 

};
#endif // SHORTCUT_H