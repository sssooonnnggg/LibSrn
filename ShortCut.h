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

	// 构造函数
	ShortCut();

	// 析构
	~ShortCut();
	
	// 设置 exe 文件路径
	VOID SetExePath(LPCWSTR lpwzExePath);

	// 设置快捷方式路径
	VOID SetLinkPath(LPCWSTR lpwzLinkPath);

	// 设置快捷键
	// eg： SetHotKey(MAKEWORD(VK_F12, HOTKEYF_CONTROL))
	// 设置快捷键为 Ctrl + F7
	VOID SetHotKey(WORD wHotKey);

	// 创建快捷方式
	BOOL CreateShortCut();

private:
	WCHAR m_wzExePath[MAX_PATH];
	WCHAR m_wzLinkPath[MAX_PATH];
	WORD m_wHotKey; 

};
#endif // SHORTCUT_H