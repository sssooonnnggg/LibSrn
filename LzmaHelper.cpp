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
// ѹ�� buffer , buffer �ɵ����߷�����ͷ�
// ���� �� ѹ��ǰ�� buffer
//         ѹ��ǰ�Ĵ�С
//         ѹ����� buffer
//         ����Ŀ�껺�����Ĵ�С������ѹ����� buffer ��С
//
BOOL LzmaHelper::CompressBuffer(
								IN PBYTE pbSrc, 
								IN DWORD dwSrcLen, 
								OUT PBYTE pbDest, 
								IN OUT LPDWORD lpdwDestLen)
{
	// ��ʼ��ѹ������
	CLzmaEncProps stProps; 
	LzmaEncProps_Init(&stProps); 

	// ����ѹ������
	stProps.level = m_dwLevel; 
	stProps.dictSize = 1 << 24; 

	// ��������
	SizeT nPropsBufLen = LZMA_PROPS_SIZE; 

	// ���估�ͷ��ڴ�ĺ���ָ�룬�� LzmaEncode ʹ��
	ISzAlloc stAllocator = { SzAlloc, SzFree }; 

	// ��ʼѹ��
	SRes rc = LzmaEncode(
		pbDest + LZMA_PROPS_SIZE,	// ǰ 5 �ֽ����ڴ�Ų��� 
		(SizeT*)lpdwDestLen,		// Ŀ�껺������С
		(unsigned char*)pbSrc,		// ѹ��ǰ������
		dwSrcLen,					// ѹ��ǰ�����ݳ���
		&stProps,					// ѹ������
		pbDest,						// ����
		&nPropsBufLen,				// ��������
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
		// ʵ�ʳ���Ϊͷ����Ϣ�������ݳ���
		*lpdwDestLen += LZMA_PROPS_SIZE;
		return TRUE;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// ��ѹ buffer , buffer �ɵ����߷�����ͷ�
// ���� �� ��ѹǰ�� buffer
//         ��ѹǰ�Ĵ�С
//         ��ѹ��� buffer
//         ���� pbDest �Ĵ�С�����ؽ�ѹ��Ĵ�С
//
BOOL LzmaHelper::UnCompressBuffer(
								  IN PBYTE pbSrc, 
								  IN DWORD dwSrcLen, 
								  OUT PBYTE pbDest, 
								  OUT LPDWORD lpdwDestLen)
{
	// �����洢ͷ����Ϣ
	unsigned int nPropsBufLen = LZMA_PROPS_SIZE; 

	// ���估�ͷ��ڴ�ĺ���ָ�룬�� LzmaDecode ʹ��
	ISzAlloc stAllocator = { SzAlloc, SzFree }; 

	// �洢��ѹ״̬
	ELzmaStatus nStatus; 

	// ��ѹǰ�����ݳ��� �� �ܳ��ȼ�ȥͷ������ )
	DWORD dwSrcLenReal = dwSrcLen - LZMA_PROPS_SIZE;
	
	SRes rc =  LzmaDecode(
		pbDest,										// ��ѹ��� buffer
		(SizeT*)lpdwDestLen,						// ��ѹ��ĳ���
		pbSrc + LZMA_PROPS_SIZE,					// ��ѹǰ������
		(SizeT*)&dwSrcLenReal,						// ��ѹǰ�����ݳ���
		pbSrc,										// �洢ͷ����Ϣ
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
// ѹ���ļ�
// ���� �� Ҫѹ�����ļ���
//		   ѹ������ļ���
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
	
	// ѹ�� buffer
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
// ��ѹ�ļ� 
// ���� �� Ҫ��ѹ���ļ���
//         ��ѹ����ļ���
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
	
	// ��ѹ�� buffer
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
// ��ȡ�ļ��� buffer ���ɵ������ͷ� 
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

	// ���ļ��� buffer
	do
	{
		ReadFile(hFile, lpBuffer + dwRead, *lpdwBufferLen, &dwRead, NULL);
	} while ( dwRead != 0 );

	CloseHandle(hFile);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////////
// �� buffer д�����ļ� 
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

	// ���ļ��� buffer
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