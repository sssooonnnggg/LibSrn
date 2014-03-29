/*!
	Description : This class is used to inject a dll or a piece of code
	              using "CreateRemoteThread" function.
	Author		: Ruining Song
	Date		: 2013.9.16
	Remark		:
			2013.10.29 >> 添加 InjectCode 函数，使用时需要注意：
						  1. 必须编译为 Release 版本，否则目标进程会崩溃
						  2. 所注入的线程中用到的所有的 api 地址和字符串必须由参数传入
						     （否则会发生重定位错误）
*/

#ifndef REMOTEINJECTER_H
#define REMOTEINJECTER_H

#include <Windows.h>

class RemoteThreadInjecter
{
public:

	typedef DWORD (WINAPI *THREADFUNC)(LPVOID lpParam);

	// 注入 dll
	BOOL InjectDll(LPCWSTR wzTargetName, LPCWSTR wzDllName);

	// 注入代码
	// 参数：
	// wzTargetName : 目标进程名
	// lpThreadFunc : 要注入的线程地址
	// lpFuncPara	: 传给线程的参数地址，一般是个结构体
	// dwParaSize	: 参数所占空间大小
	BOOL InjectCode(LPWSTR wzTargetName, THREADFUNC lpThreadFunc, PVOID lpFuncPara, DWORD dwParaSize);

private:
	// 由进程名得到进程句柄
	HANDLE GetProcessHandleByName(LPCWSTR wzProcessName);

	// 提升进程权限
	BOOL EnableDebugPrivilege();
};

#endif // REMOTEINJECTER_H