/*!
	Decription : This class used to print debug infomation.
	Author     : Ruining Song
	Date       : 2013.9.17
	Remark     :
*/

#ifndef DEBUGTOOLS_H
#define DEBUGTOOLS_H

#include <Windows.h>

class DebugTools
{
public:

#ifdef UNICODE
#define OutputDebugPrintf OutputDebugPrintfW
#else
#define OutputDebugPrintf OutputDebugPrintfA
#endif

	// 输出调试信息
	inline static void OutputDebugPrintfW(LPCWSTR ptzFormat, ...)
	{
#ifdef _DEBUG
		va_list vlArgs;
		WCHAR szText[1024];
		va_start(vlArgs, ptzFormat);
		wvsprintfW(szText, ptzFormat, vlArgs);
		OutputDebugStringW(szText);
		va_end(vlArgs);
#endif
	}

	inline static void OutputDebugPrintfA(LPCSTR ptzFormat, ...)
	{
#ifdef _DEBUG
		va_list vlArgs;
		CHAR szText[1024];
		va_start(vlArgs, ptzFormat);
		wvsprintfA(szText, ptzFormat, vlArgs);
		OutputDebugStringA(szText);
		va_end(vlArgs);
#endif
	}


};

#endif // DEBUGTOOLS_H