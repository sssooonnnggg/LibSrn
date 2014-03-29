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
	// ���ע���ű�
	// �ű�������һ����ִ���ļ�����ͬ
	BOOL AddLogOffScript(LPCWSTR wzScriptPath);

	// ��ӿ����ű�
	BOOL AddStartScript(LPCWSTR wzScriptPath);

private:
	GpoToolImpl* m_pGpoToolImpl;
};
#endif // GPOTOOLS_H