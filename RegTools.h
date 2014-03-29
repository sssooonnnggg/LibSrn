/*!
	Decription : Useful Registry Utils.
	Author     : Ruining Song
	Date       : 2013.9.17
	Remark     :
			2013-9-27 >> ����� RegGetPrivileg RegGetAllAccess
*/

#ifndef REGTOOLS_H
#define REGTOOLS_H

#include <Windows.h>

class RegTools
{
public:

	// �޸�ע����ַ�����ֵ
	BOOL RegSetString(
		HKEY hkRoot, 
		LPCWSTR wzLocation, 
		LPCWSTR wzName, 
		LPCWSTR wzValue);

	// �������̲���ע����Ȩ��
	BOOL RegGetPrivilege();

	// ������ǰ�˻�����ע����Ȩ��
	BOOL RegGetAllAccess(LPCWSTR lpwzSamName);
};

#endif // REGTOOLS_H