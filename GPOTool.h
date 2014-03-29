/*!
	Description : Add Group Policy log off/start scripts.
	Author		: Ruining Song
	Date		: 2013.9.25
	Remark		:
*/

#ifndef GPOTOOLS_H
#define GPOTOOLS_H

#include <windows.h>

class GpoToolImpl;

class GpoTool
{
public:
	GpoTool();
	~GpoTool();

public:
	// 添加注销脚本
	// 脚本可以是一个可执行文件，下同
	BOOL AddLogOffScript(LPCWSTR wzScriptPath);

	// 添加开机脚本
	BOOL AddStartScript(LPCWSTR wzScriptPath);

private:
	GpoToolImpl* m_pGpoToolImpl;
};
#endif // GPOTOOLS_H