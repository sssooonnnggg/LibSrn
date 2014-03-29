/*!
	Description : Used To Load A Dll From Memory.
	Author		: Ruining Song
	Date		: 2013.10.29
	Remark		: Dll 导出函数必须用 extern "C" 导出
*/

#ifndef MEMORYLOADER_H
#define MEMORYLOADER_H

#include <Windows.h>

typedef void* HMEMORYMODULE;

class MemoryLoader
{
public:

	// 从内存中加载 动态库
	// 参数 ：内存指针
	// 返回 ：Dll 句柄
	static HMEMORYMODULE MemoryLoadLibrary(const void *);

	// 取得 Dll 导出函数
	// 参数： dll 句柄，函数名
	// 返回： 函数指针
	static FARPROC MemoryGetProcAddress(HMEMORYMODULE, const char *);

	// 释放动态库
	static VOID MemoryFreeLibrary(HMEMORYMODULE);

};
#endif // MEMORYLOADER_H