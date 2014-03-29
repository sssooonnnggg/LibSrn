/*!
	Description : Used To Load A Dll From Memory.
	Author		: Ruining Song
	Date		: 2013.10.29
	Remark		: Dll 导出函数必须用 extern "C" 导出
*/

#include "stdafx.h"
#include "MemoryLoader.h"
#include "DebugTools.h"

#include <winnt.h>

// ////////////////////////////////////////////////////////////////////////////////
// 32 / 64 位预处理 
//
#ifdef _WIN64
#define POINTER_TYPE ULONGLONG
#else
#define POINTER_TYPE DWORD
#endif

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))
#endif

#ifdef _WIN64
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
#endif

typedef struct {
	PIMAGE_NT_HEADERS headers;
	unsigned char *codeBase;
	HMODULE *modules;
	int numModules;
	int initialized;
} MEMORYMODULE, *PMEMORYMODULE;

// ////////////////////////////////////////////////////////////////////////////////
// winnt.h 中 IMAGE_SECTION_HEADER 定义有误，应修正为 
//
//typedef struct _IMAGE_SECTION_HEADER {
//	BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
//	union {
//		DWORD   PhysicalAddress;
//		DWORD   VirtualSize;
//	} Misc;
//	DWORD   VirtualAddress;
//	DWORD   SizeOfRawData;
//	DWORD   PointerToRawData;
//	DWORD   PointerToRelocations;
//	DWORD   PointerToLinenumbers;
//	WORD    NumberOfRelocations;
//	WORD    NumberOfLinenumbers;
//	DWORD   Characteristics;
//} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

// ////////////////////////////////////////////////////////////////////////////////
// Dll 入口点 
//
typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#define GET_HEADER_DICTIONARY(module, idx)	&(module)->headers->OptionalHeader.DataDirectory[idx]

// ////////////////////////////////////////////////////////////////////////////////
// 保护标志位 
//
static int ProtectionFlags[2][2][2] = {
	{
		// 不可执行
		{PAGE_NOACCESS, PAGE_WRITECOPY},
		{PAGE_READONLY, PAGE_READWRITE},
	}, {
		// 可执行
		{PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
		{PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
	},
};

// ////////////////////////////////////////////////////////////////////////////////
// @private 
//
namespace MemoryLoaderPrivate
{
	// ////////////////////////////////////////////////////////////////////////////////
	// 将 Section 中的数据复制到 VirtualAlloc 分配的虚拟内存中 
	//
	static void CopySections(const unsigned char *data, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
	{
		int i, size;
		unsigned char *codeBase = module->codeBase;
		unsigned char *dest;

		// 第一个 Section
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);

		// 遍历所有 Section
		for ( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) 
		{
			if ( section->SizeOfRawData == 0 ) 
			{
				// 如果 SizeOfRawData 为 0,
				// 分配 SectionAlignment 大小的空间
				size = old_headers->OptionalHeader.SectionAlignment;
				if ( size > 0 ) 
				{
					dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
						size,
						MEM_COMMIT,
						PAGE_READWRITE);

					section->Misc.PhysicalAddress = (POINTER_TYPE)dest;
					memset(dest, 0, size);
				}

				continue;
			}

			// 将 Section 中数据复制过去
			dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
				section->SizeOfRawData,
				MEM_COMMIT,
				PAGE_READWRITE);
			memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
			section->Misc.PhysicalAddress = (POINTER_TYPE)dest;
		}

	}

	// ////////////////////////////////////////////////////////////////////////////////
	// 复制之后对 Section 的处理 
	//
	static void FinalizeSections(PMEMORYMODULE module)
	{
		int i;
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
#ifdef _WIN64
		POINTER_TYPE imageOffset = (module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
		POINTER_TYPE imageOffset = 0x0;
#endif

		// 遍历 Section，修改标志位
		for ( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
			DWORD protect, oldProtect, size;
			int executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
			int readable   = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
			int writeable  = (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

			if ( section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE ) 
			{
				// 可释放的 Section
				VirtualFree((LPVOID)((POINTER_TYPE)section->Misc.PhysicalAddress | imageOffset), section->SizeOfRawData, MEM_DECOMMIT);
				continue;
			}

			// 设置标志位
			protect = ProtectionFlags[executable][readable][writeable];
			if ( section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED ) 
			{
				protect |= PAGE_NOCACHE;
			}

			// 设置要保护的区域的大小
			size = section->SizeOfRawData;
			if (size == 0) {
				if ( section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA ) 
				{
					size = module->headers->OptionalHeader.SizeOfInitializedData;
				} 
				else if ( section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ) 
				{
					size = module->headers->OptionalHeader.SizeOfUninitializedData;
				}
			}

			if ( size > 0 ) 
			{
				// 修改 Flag
				if ( VirtualProtect(
					(LPVOID)((POINTER_TYPE)section->Misc.PhysicalAddress | imageOffset), 
					size, 
					protect, 
					&oldProtect) == 0 )
				{
					DebugTools::OutputDebugPrintf(L"[MemoryLoader] [FinalizeSections] Error Protecting Memory Page.\r\n");
				}
			}
		}
	}

	// ////////////////////////////////////////////////////////////////////////////////
	// 修改重定位表 
	//
	static void PerformBaseRelocation(PMEMORYMODULE module, ULONGLONG delta)
	{
		DWORD i;
		unsigned char *codeBase = module->codeBase;
		PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);

		if (directory->Size > 0) 
		{
			PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);

			for ( int number = 1; relocation->VirtualAddress > 0; number++ ) 
			{
				unsigned char *dest = codeBase + relocation->VirtualAddress;
				unsigned short *relInfo = (unsigned short *)((unsigned char *)relocation + IMAGE_SIZEOF_BASE_RELOCATION);
				int j = 0;

				for ( i = 0; i < ((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION)/2); i++, relInfo++ ) 
				{
					DWORD *patchAddrHL;
#ifdef _WIN64
					ULONGLONG *patchAddr64;
#endif
					int type, offset;

					// 高 4 位代表重定位的类型
					type = *relInfo >> 12;
					// 低 12 位代表偏移
					offset = *relInfo & 0xfff;

					switch (type)
					{
					case IMAGE_REL_BASED_ABSOLUTE:
						break;

					case IMAGE_REL_BASED_HIGHLOW:
						// 修改地址
						patchAddrHL = (DWORD *) (dest + offset);
						*patchAddrHL += (DWORD)delta;
						break;

#ifdef _WIN64
					case IMAGE_REL_BASED_DIR64:
						patchAddr64 = (ULONGLONG *) (dest + offset);
						*patchAddr64 += (ULONGLONG) delta;
						break;
#endif

					default:
						break;
					}
				}

				// 下一个重定位块
				relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
			}
		}
	}

	// ////////////////////////////////////////////////////////////////////////////////
	// 构建输入表 
	//
	static int BuildImportTable(PMEMORYMODULE module)
	{
		int result=1;
		unsigned char *codeBase = module->codeBase;

		PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
		if ( directory->Size > 0) 
		{
			PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);

			for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++) 
			{
				POINTER_TYPE *thunkRef;
				FARPROC *funcRef;
				HMODULE handle = LoadLibraryA((LPCSTR) (codeBase + importDesc->Name));

				if ( handle == NULL ) 
				{
					result = 0;
					break;
				}

				module->modules = (HMODULE *)realloc(module->modules, (module->numModules+1)*(sizeof(HMODULE)));

				if ( module->modules == NULL ) 
				{
					result = 0;
					break;
				}

				module->modules[module->numModules++] = handle;

				if ( importDesc->OriginalFirstThunk ) 
				{
					thunkRef = (POINTER_TYPE *) (codeBase + importDesc->OriginalFirstThunk);
					funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
				} 
				else 
				{
					// no hint table
					thunkRef = (POINTER_TYPE *) (codeBase + importDesc->FirstThunk);
					funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
				}

				for (; *thunkRef; thunkRef++, funcRef++) 
				{
					if ( IMAGE_SNAP_BY_ORDINAL(*thunkRef) ) 
					{
						*funcRef = (FARPROC)GetProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef));
					} 
					else 
					{
						PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
						*funcRef = (FARPROC)GetProcAddress(handle, (LPCSTR)&thunkData->Name);
					}

					if ( *funcRef == 0 ) 
					{
						result = 0;
						break;
					}
				}

				if (!result) 
				{
					break;
				}
			}
		}

		return result;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// 加载动态库 
//
HMEMORYMODULE MemoryLoader::MemoryLoadLibrary(const void *data)
{
	PMEMORYMODULE result;
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS old_header;

	unsigned char *code, *headers;
	ULONGLONG locationDelta;
	DllEntryProc DllEntry;
	BOOL successfull;

	dos_header = (PIMAGE_DOS_HEADER)data;

	// 检查 Dll 合法性
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) 
	{
		WCHAR aa[MAX_PATH] = {0};
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryLoadLibrary] Not A Valid Dll File.\r\n");
		return NULL;
	}

	old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *)(data))[dos_header->e_lfanew];
	if (old_header->Signature != IMAGE_NT_SIGNATURE) 
	{
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryLoadLibrary] No PE Header Found.\r\n");
		return NULL;
	}

	// 指定基址上分配内存 （MEM_RESERVE)
	code = (unsigned char *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
		old_header->OptionalHeader.SizeOfImage,
		MEM_RESERVE,
		PAGE_READWRITE);

	// 若基址已被占用，则重新分配，此时需要做重定位处理
	if ( code == NULL ) 
	{
		code = (unsigned char *)VirtualAlloc(
			NULL,
			old_header->OptionalHeader.SizeOfImage,
			MEM_RESERVE,
			PAGE_READWRITE);

		CHAR out[MAX_PATH] = {0};

		if (code == NULL) 
		{
			DebugTools::OutputDebugPrintf(
				L"[MemoryLoader] [MemoryLoadLibrary] Reserve Memory Failed.\r\n");
			return NULL;
		}
	}

	result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), 0, sizeof(MEMORYMODULE));
	result->codeBase = code;
	result->numModules = 0;
	result->modules = NULL;
	result->initialized = 0;

	// 提交内存 (MEM_COMMIT)
	VirtualAlloc(
		code,
		old_header->OptionalHeader.SizeOfImage,
		MEM_COMMIT,
		PAGE_READWRITE);

	// 提交 Headers 内存
	headers = (unsigned char *)VirtualAlloc(
		code,
		dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders,
		MEM_COMMIT,
		PAGE_READWRITE);

	// 复制 PE Header
	memcpy(headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders);
	result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

	// 更新基址
	result->headers->OptionalHeader.ImageBase = (POINTER_TYPE)code;

	// 复制 Section
	MemoryLoaderPrivate::CopySections((const unsigned char *)data, old_header, result);

	// 重定位
	locationDelta = (ULONGLONG)code - old_header->OptionalHeader.ImageBase;
	if ( locationDelta != 0 ) 
	{
		MemoryLoaderPrivate::PerformBaseRelocation(result, locationDelta);
	}

	// 构建输入表
	if ( !MemoryLoaderPrivate::BuildImportTable(result) ) 
	{
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryLoadLibrary] BuildImportTable Failed.\r\n");
		goto error;
	}
	
	// 修改内存保护标志位
	MemoryLoaderPrivate::FinalizeSections(result);

	// 调用 DllMain
	if ( result->headers->OptionalHeader.AddressOfEntryPoint != 0 ) 
	{

		DllEntry = (DllEntryProc) (code + result->headers->OptionalHeader.AddressOfEntryPoint);

		if ( DllEntry == 0 ) 
		{
			DebugTools::OutputDebugPrintf(
				L"[MemoryLoader] [MemoryLoadLibrary] DllMain Not Found.\r\n");
			goto error;
		}

		successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0);
		if ( !successfull ) 
		{
			// DllMain 返回 FALSE
			DebugTools::OutputDebugPrintf(
				L"[MemoryLoader] [MemoryLoadLibrary] Dll Attach Failed.\r\n");
			goto error;
		}

		result->initialized = 1;
	}

	return (HMEMORYMODULE)result;

error:
	MemoryFreeLibrary(result);
	return NULL;
}


// ////////////////////////////////////////////////////////////////////////////////
// 获取函数地址 
//
FARPROC MemoryLoader::MemoryGetProcAddress(HMEMORYMODULE module, const char *name)
{
	unsigned char *codeBase = ((PMEMORYMODULE)module)->codeBase;
	int idx = -1;
	DWORD i, *nameRef;
	WORD *ordinal;
	PIMAGE_EXPORT_DIRECTORY exports;
	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE)module, IMAGE_DIRECTORY_ENTRY_EXPORT);

	if ( directory->Size == 0 ) 
	{
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryGetProcAddress] No Export Table Found.\r\n");
		return NULL;
	}

	exports = (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
	if ( exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0 ) 
	{
		return NULL;
	}

	// 搜索导出函数名
	nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
	ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);
	for ( i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++ ) 
	{
		if ( lstrcmpiA(name, (const char *) (codeBase + (*nameRef))) == 0 ) 
		{
			idx = *ordinal;
			break;
		}
	}

	if ( idx == -1 ) 
	{
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryGetProcAddress] No Function Compatible Found.\r\n");
		return NULL;
	}

	if ( (DWORD)idx > exports->NumberOfFunctions )
	{
		return NULL;
	}

	return (FARPROC) (codeBase + (*(DWORD *) (codeBase + exports->AddressOfFunctions + (idx*4))));
}


// ////////////////////////////////////////////////////////////////////////////////
// 释放动态库 
//
VOID MemoryLoader::MemoryFreeLibrary(HMEMORYMODULE mod)
{
	int i;
	PMEMORYMODULE module = (PMEMORYMODULE)mod;

	if ( module != NULL ) 
	{
		if ( module->initialized != 0 ) 
		{
			// 调用 DllMain 
			// DLL_PROCESS_DETACH
			DllEntryProc DllEntry = (DllEntryProc) (module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
			(*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
			module->initialized = 0;
		}

		if ( module->modules != NULL )  
		{
			// 释放此 Dll 加载的其他 Dll
			for ( i = 0; i < module->numModules; i++ ) 
			{
				if ( module->modules[i] != INVALID_HANDLE_VALUE ) 
				{
					FreeLibrary(module->modules[i]);
				}
			}

			free(module->modules);
		}

		if (module->codeBase != NULL) 
		{
			// 释放 Dll 占用的内存
			VirtualFree(module->codeBase, 0, MEM_RELEASE);
		}

		HeapFree(GetProcessHeap(), 0, module);
	}
}
