/*!
	Description : This class is used for compress/uncompress data using the lzma algorithm.
	Author		: Ruining Song
	Date		: 2013.8.27
	Remark		:
*/

#ifndef LZMAHELPER_H
#define LZMAHELPER_H

#include <Windows.h>
#include <stdio.h>

#include "Lzma/LzmaDec.h"
#include "Lzma/LzmaEnc.h"


class LzmaHelper
{
public:
	
	LzmaHelper():m_dwLevel(9){};

	// ѹ������
	BOOL CompressBuffer(
		IN PBYTE pbSrc, 
		IN DWORD dwSrcLen, 
		OUT PBYTE pbDest, 
		IN OUT LPDWORD lpdwDestLen);

	// ��ѹ����
	BOOL UnCompressBuffer(
		IN PBYTE pbSrc, 
		IN DWORD dwSrcLen, 
		OUT PBYTE pbDest, 
		OUT LPDWORD lpdwDestLen);

	// ѹ���ļ�
	BOOL CompressFile(IN LPWSTR wzSrcFileName, OUT LPWSTR wzDestFileName);

	// ��ѹ�ļ�
	BOOL UnCompressFile(IN LPWSTR wzSrcFileName, OUT LPWSTR wzDestFileName);

	// ����ѹ������ �� 1 - 9 ��
	inline BOOL SetCompressLevel(DWORD dwLevel)
	{
		return (dwLevel > 0 && dwLevel < 10) ?
			(m_dwLevel = dwLevel) : FALSE;
	}

private:
	static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
	static void SzFree(void *p, void *address) { p = p; free(address); }

	// ���ļ��� buffer���ɵ������ͷ�
	BOOL ReadFileToBuffer(LPWSTR wzFileName, PBYTE& lpBuffer, PDWORD lpdwBufferLen);

	// д buffer ���ļ�
	BOOL WriteBufferToFile(LPWSTR wzFileName, PBYTE lpBuffer, DWORD dwBufferLen);

private:
	DWORD m_dwLevel;
};

#endif // LZMAHELPER_H