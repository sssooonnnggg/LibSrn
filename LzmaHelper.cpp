/*!
	Description : This class is used for compress/uncompress data using the lzma algorithm.
	Author		: Ruining Song
	Date		: 2013.8.27
	Remark		:
*/

#include "stdafx.h"

#include "DebugTools.h"
#include "LzmaHelper.h"

// ////////////////////////////////////////////////////////////////////////////////
// 压缩 buffer , buffer 由调用者分配和释放
// 参数 ： 压缩前的 buffer
//         压缩前的大小
//         压缩后的 buffer
//         传入目标缓冲区的大小，返回压缩后的 buffer 大小
//
BOOL LzmaHelper::CompressBuffer(
								IN PBYTE pbSrc, 
								IN DWORD dwSrcLen, 
								OUT PBYTE pbDest, 
								IN OUT LPDWORD lpdwDestLen)
{
	// 初始化压缩参数
	CLzmaEncProps stProps; 
	LzmaEncProps_Init(&stProps); 

	// 设置压缩参数
	stProps.level = m_dwLevel; 
	stProps.dictSize = 1 << 24; 

	// 参数长度
	SizeT nPropsBufLen = LZMA_PROPS_SIZE; 

	// 分配及释放内存的函数指针，供 LzmaEncode 使用
	ISzAlloc stAllocator = { SzAlloc, SzFree }; 

	// 开始压缩
	SRes rc = LzmaEncode(
		pbDest + LZMA_PROPS_SIZE,	// 前 5 字节用于存放参数 
		(SizeT*)lpdwDestLen,		// 目标缓冲区大小
		(unsigned char*)pbSrc,		// 压缩前的数据
		dwSrcLen,					// 压缩前的数据长度
		&stProps,					// 压缩参数
		pbDest,						// 参数
		&nPropsBufLen,				// 参数长度
		0, 
		NULL, 
		&stAllocator, 
		&stAllocator);
	
	if ( SZ_OK != rc )
	{ 
		DebugTools::OutputDebugPrintf(L"[LzmaHelper] [CompressBuffer] CompressBuffer Failed.[%d]\r\n", rc);
		return FALSE;	
	}
	else
	{
		// 实际长度为头部信息加上数据长度
		*lpdwDestLen += LZMA_PROPS_SIZE;
		return TRUE;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// 解压 buffer , buffer 由调用者分配和释放
// 参数 ： 解压前的 buffer
//         解压前的大小
//         解压后的 buffer
//         传入 pbDest 的大小，返回解压后的大小
//
BOOL LzmaHelper::UnCompressBuffer(
								  IN PBYTE pbSrc, 
								  IN DWORD dwSrcLen, 
								  OUT PBYTE pbDest, 
								  OUT LPDWORD lpdwDestLen)
{
	// 用来存储头部信息
	unsigned int nPropsBufLen = LZMA_PROPS_SIZE; 

	// 分配及释放内存的函数指针，供 LzmaDecode 使用
	ISzAlloc stAllocator = { SzAlloc, SzFree }; 

	// 存储解压状态
	ELzmaStatus nStatus; 

	// 解压前的数据长度 （ 总长度减去头部长度 )
	DWORD dwSrcLenReal = dwSrcLen - LZMA_PROPS_SIZE;
	
	SRes rc =  LzmaDecode(
		pbDest,										// 解压后的 buffer
		(SizeT*)lpdwDestLen,						// 解压后的长度
		pbSrc + LZMA_PROPS_SIZE,					// 解压前的数据
		(SizeT*)&dwSrcLenReal,						// 解压前的数据长度
		pbSrc,										// 存储头部信息
		nPropsBufLen,							
		LZMA_FINISH_ANY, 
		&nStatus, 
		&stAllocator);

	if ( SZ_OK != rc )
	{ 
		DebugTools::OutputDebugPrintf(L"[LzmaHelper] [UnCompressBuffer] UnCompressBuffer Failed.[%d]\r\n", rc);
		return FALSE;	
	}
	else
	{
		return TRUE;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// 压缩文件
// 参数 ： 要压缩的文件名
//		   压缩后的文件名
//
BOOL LzmaHelper::CompressFile(IN LPWSTR wzSrcFileName, OUT LPWSTR wzDestFileName)
{
	PBYTE lpSrc = NULL;
	DWORD dwSrcLen = 0;
	BOOL bRet = TRUE;

	if ( !ReadFileToBuffer(wzSrcFileName, lpSrc, &dwSrcLen) )
	{
		return FALSE;
	}
	
	// 压缩 buffer
	PBYTE lpDest = new BYTE[dwSrcLen + 100];
	memset(lpDest, 0, dwSrcLen + 100);
	DWORD dwDestLen = dwSrcLen + 100;

	if ( !CompressBuffer(lpSrc, dwSrcLen, lpDest, &dwDestLen) )
	{
		bRet = FALSE;
		goto CLEAN;
	}


	if ( !WriteBufferToFile(wzDestFileName, lpDest, dwDestLen) )
	{
		bRet = FALSE;
	}

CLEAN:
	delete [] lpSrc;
	delete [] lpDest;
	return bRet;
}

// ////////////////////////////////////////////////////////////////////////////////
// 解压文件 
// 参数 ： 要解压的文件名
//         解压后的文件名
//
BOOL LzmaHelper::UnCompressFile(IN LPWSTR wzSrcFileName, OUT LPWSTR wzDestFileName)
{
	PBYTE lpSrc = NULL;
	DWORD dwSrcLen = 0;
	BOOL bRet = TRUE;

	if ( !ReadFileToBuffer(wzSrcFileName, lpSrc, &dwSrcLen) )
	{
		return FALSE;
	}
	
	// 解压缩 buffer
	PBYTE lpSrcBuf = new BYTE[dwSrcLen * 20];
	memset(lpSrcBuf, 0, dwSrcLen * 20);
	memcpy(lpSrcBuf, lpSrc, dwSrcLen);
	PBYTE lpDest = new BYTE[dwSrcLen * 20];
	memset(lpDest, 0, dwSrcLen * 20);
	DWORD dwDestLen = dwSrcLen * 20;

	if ( !UnCompressBuffer(lpSrcBuf, dwSrcLen, lpDest, &dwDestLen) )
	{
		bRet = FALSE;
		goto CLEAN;
	}

	if ( !WriteBufferToFile(wzDestFileName, lpDest, dwDestLen) )
	{
		bRet = FALSE;
	}

CLEAN:
	delete [] lpSrc;
	delete [] lpSrcBuf;
	delete [] lpDest;
	return bRet;
}

// ////////////////////////////////////////////////////////////////////////////////
// 读取文件至 buffer ，由调用者释放 
//
BOOL LzmaHelper::ReadFileToBuffer(LPWSTR wzFileName, PBYTE& lpBuffer, PDWORD lpdwBufferLen)
{
	HANDLE hFile = CreateFileW(
		wzFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if ( INVALID_HANDLE_VALUE == hFile )
	{
		DebugTools::OutputDebugPrintf(L"[LzmaHelper] [ReadFileToBuffer] Can't Open File [%s]\r\n", wzFileName);
		return FALSE;
	}
	
	DWORD dwDump = 0;
	*lpdwBufferLen = GetFileSize(hFile, &dwDump);
	lpBuffer = new BYTE[*lpdwBufferLen];
	memset(lpBuffer, 0, *lpdwBufferLen);
	DWORD dwRead = 0;

	// 读文件至 buffer
	do
	{
		ReadFile(hFile, lpBuffer + dwRead, *lpdwBufferLen, &dwRead, NULL);
	} while ( dwRead != 0 );

	CloseHandle(hFile);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// 将 buffer 写入至文件 
//
BOOL LzmaHelper::WriteBufferToFile(LPWSTR wzFileName, PBYTE lpBuffer, DWORD dwBufferLen)
{
	HANDLE hFile = CreateFileW(
		wzFileName,
		GENERIC_WRITE,
		NULL,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if ( INVALID_HANDLE_VALUE == hFile )
	{
		DebugTools::OutputDebugPrintf(L"[LzmaHelper] [WriteBufferToFile] Can't Open File [%s]\r\n", wzFileName);
		return FALSE;
	}
	
	DWORD dwWrite = 0;
	DWORD dwWriteTotal = 0;

	// 读文件至 buffer
	do
	{
		WriteFile(
			hFile, 
			lpBuffer + dwWriteTotal, 
			dwBufferLen - dwWriteTotal, 
			&dwWrite, NULL);
		dwWriteTotal += dwWrite;
	} while ( dwWriteTotal != dwBufferLen );

	CloseHandle(hFile);
	return TRUE;
}