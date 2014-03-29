/*!
	Description : Used To Load A Dll From Memory.
	Author		: Ruining Song
	Date		: 2013.10.29
	Remark		: Dll �������������� extern "C" ����
*/

#ifndef MEMORYLOADER_H
#define MEMORYLOADER_H

#include <Windows.h>

typedef void* HMEMORYMODULE;

class MemoryLoader
{
public:

	// ���ڴ��м��� ��̬��
	// ���� ���ڴ�ָ��
	// ���� ��Dll ���
	static HMEMORYMODULE MemoryLoadLibrary(const void *);

	// ȡ�� Dll ��������
	// ������ dll �����������
	// ���أ� ����ָ��
	static FARPROC MemoryGetProcAddress(HMEMORYMODULE, const char *);

	// �ͷŶ�̬��
	static VOID MemoryFreeLibrary(HMEMORYMODULE);

};
#endif // MEMORYLOADER_H