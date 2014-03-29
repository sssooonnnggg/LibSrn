/*!
	Description : This class is used to inject a dll or a piece of code
	              using "CreateRemoteThread" function.
	Author		: Ruining Song
	Date		: 2013.9.16
	Remark		:
			2013.10.29 >> ��� InjectCode ������ʹ��ʱ��Ҫע�⣺
						  1. �������Ϊ Release �汾������Ŀ����̻����
						  2. ��ע����߳����õ������е� api ��ַ���ַ��������ɲ�������
						     ������ᷢ���ض�λ����
*/

#ifndef REMOTEINJECTER_H
#define REMOTEINJECTER_H

#include <Windows.h>

class RemoteThreadInjecter
{
public:

	typedef DWORD (WINAPI *THREADFUNC)(LPVOID lpParam);

	// ע�� dll
	BOOL InjectDll(LPCWSTR wzTargetName, LPCWSTR wzDllName);

	// ע�����
	// ������
	// wzTargetName : Ŀ�������
	// lpThreadFunc : Ҫע����̵߳�ַ
	// lpFuncPara	: �����̵߳Ĳ�����ַ��һ���Ǹ��ṹ��
	// dwParaSize	: ������ռ�ռ��С
	BOOL InjectCode(LPWSTR wzTargetName, THREADFUNC lpThreadFunc, PVOID lpFuncPara, DWORD dwParaSize);

private:
	// �ɽ������õ����̾��
	HANDLE GetProcessHandleByName(LPCWSTR wzProcessName);

	// ��������Ȩ��
	BOOL EnableDebugPrivilege();
};

#endif // REMOTEINJECTER_H