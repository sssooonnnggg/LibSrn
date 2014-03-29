/*!
	Description : Used To Load A Dll From Memory.
	Author		: Ruining Song
	Date		: 2013.10.29
	Remark		: Dll �������������� extern "C" ����
*/

#include "stdafx.h"
#include "MemoryLoader.h"
#include "DebugTools.h"

#include <winnt.h>

// ////////////////////////////////////////////////////////////////////////////////
// 32 / 64 λԤ���� 
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
// winnt.h �� IMAGE_SECTION_HEADER ��������Ӧ����Ϊ 
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
// Dll ��ڵ� 
//
typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#define GET_HEADER_DICTIONARY(module, idx)	&(module)->headers->OptionalHeader.DataDirectory[idx]

// ////////////////////////////////////////////////////////////////////////////////
// ������־λ 
//
static int ProtectionFlags[2][2][2] = {
	{
		// ����ִ��
		{PAGE_NOACCESS, PAGE_WRITECOPY},
		{PAGE_READONLY, PAGE_READWRITE},
	}, {
		// ��ִ��
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
	// �� Section �е����ݸ��Ƶ� VirtualAlloc ����������ڴ��� 
	//
	static void CopySections(const unsigned char *data, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
	{
		int i, size;
		unsigned char *codeBase = module->codeBase;
		unsigned char *dest;

		// ��һ�� Section
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);

		// �������� Section
		for ( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) 
		{
			if ( section->SizeOfRawData == 0 ) 
			{
				// ��� SizeOfRawData Ϊ 0,
				// ���� SectionAlignment ��С�Ŀռ�
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

			// �� Section �����ݸ��ƹ�ȥ
			dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
				section->SizeOfRawData,
				MEM_COMMIT,
				PAGE_READWRITE);
			memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
			section->Misc.PhysicalAddress = (POINTER_TYPE)dest;
		}

	}

	// ////////////////////////////////////////////////////////////////////////////////
	// ����֮��� Section �Ĵ��� 
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

		// ���� Section���޸ı�־λ
		for ( i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
			DWORD protect, oldProtect, size;
			int executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
			int readable   = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
			int writeable  = (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

			if ( section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE ) 
			{
				// ���ͷŵ� Section
				VirtualFree((LPVOID)((POINTER_TYPE)section->Misc.PhysicalAddress | imageOffset), section->SizeOfRawData, MEM_DECOMMIT);
				continue;
			}

			// ���ñ�־λ
			protect = ProtectionFlags[executable][readable][writeable];
			if ( section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED ) 
			{
				protect |= PAGE_NOCACHE;
			}

			// ����Ҫ����������Ĵ�С
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
				// �޸� Flag
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
	// �޸��ض�λ�� 
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

					// �� 4 λ�����ض�λ������
					type = *relInfo >> 12;
					// �� 12 λ����ƫ��
					offset = *relInfo & 0xfff;

					switch (type)
					{
					case IMAGE_REL_BASED_ABSOLUTE:
						break;

					case IMAGE_REL_BASED_HIGHLOW:
						// �޸ĵ�ַ
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

				// ��һ���ض�λ��
				relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
			}
		}
	}

	// ////////////////////////////////////////////////////////////////////////////////
	// ��������� 
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
// ���ض�̬�� 
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

	// ��� Dll �Ϸ���
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

	// ָ����ַ�Ϸ����ڴ� ��MEM_RESERVE)
	code = (unsigned char *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
		old_header->OptionalHeader.SizeOfImage,
		MEM_RESERVE,
		PAGE_READWRITE);

	// ����ַ�ѱ�ռ�ã������·��䣬��ʱ��Ҫ���ض�λ����
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

	// �ύ�ڴ� (MEM_COMMIT)
	VirtualAlloc(
		code,
		old_header->OptionalHeader.SizeOfImage,
		MEM_COMMIT,
		PAGE_READWRITE);

	// �ύ Headers �ڴ�
	headers = (unsigned char *)VirtualAlloc(
		code,
		dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders,
		MEM_COMMIT,
		PAGE_READWRITE);

	// ���� PE Header
	memcpy(headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders);
	result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

	// ���»�ַ
	result->headers->OptionalHeader.ImageBase = (POINTER_TYPE)code;

	// ���� Section
	MemoryLoaderPrivate::CopySections((const unsigned char *)data, old_header, result);

	// �ض�λ
	locationDelta = (ULONGLONG)code - old_header->OptionalHeader.ImageBase;
	if ( locationDelta != 0 ) 
	{
		MemoryLoaderPrivate::PerformBaseRelocation(result, locationDelta);
	}

	// ���������
	if ( !MemoryLoaderPrivate::BuildImportTable(result) ) 
	{
		DebugTools::OutputDebugPrintf(
			L"[MemoryLoader] [MemoryLoadLibrary] BuildImportTable Failed.\r\n");
		goto error;
	}
	
	// �޸��ڴ汣����־λ
	MemoryLoaderPrivate::FinalizeSections(result);

	// ���� DllMain
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
			// DllMain ���� FALSE
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
// ��ȡ������ַ 
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

	// ��������������
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
// �ͷŶ�̬�� 
//
VOID MemoryLoader::MemoryFreeLibrary(HMEMORYMODULE mod)
{
	int i;
	PMEMORYMODULE module = (PMEMORYMODULE)mod;

	if ( module != NULL ) 
	{
		if ( module->initialized != 0 ) 
		{
			// ���� DllMain 
			// DLL_PROCESS_DETACH
			DllEntryProc DllEntry = (DllEntryProc) (module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
			(*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
			module->initialized = 0;
		}

		if ( module->modules != NULL )  
		{
			// �ͷŴ� Dll ���ص����� Dll
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
			// �ͷ� Dll ռ�õ��ڴ�
			VirtualFree(module->codeBase, 0, MEM_RELEASE);
		}

		HeapFree(GetProcessHeap(), 0, module);
	}
}
