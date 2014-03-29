/*!
	Decription : Useful Registry Utils.
	Author     : Ruining Song
	Date       : 2013.9.17
	Remark     :
			2013-9-27 >> 添加了 RegGetPrivileg RegGetAllAccess
*/

#ifndef REGTOOLS_H
#define REGTOOLS_H

#include <Windows.h>

class RegTools
{
public:

	// 修改注册表字符串的值
	BOOL RegSetString(
		HKEY hkRoot, 
		LPCWSTR wzLocation, 
		LPCWSTR wzName, 
		LPCWSTR wzValue);

	// 提升进程操作注册表的权限
	BOOL RegGetPrivilege();

	// 提升当前账户操作注册表的权限
	BOOL RegGetAllAccess(LPCWSTR lpwzSamName);
};

#endif // REGTOOLS_H