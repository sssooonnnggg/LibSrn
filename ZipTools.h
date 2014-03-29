/*!
	Description : lib to compress/uncompress the .zip files.
	Author		: Ruining Song
	Date		: 2013.12.31
	Remark		:
			
			>> dependency : zlib ( zlibstat.lib MT )
*/

#ifndef ZIPTOOLS_H
#define ZIPTOOLS_H

#include <Windows.h>

class ZipTools
{

public:

	// ��ָ���ļ��д����ָ�� zip
	// eg : C:\Windows\123 -> D:\test\222.zip;
	static bool Zip(LPCWSTR wzDirPath, LPCWSTR wzDestName);

	// ��ָ�� zip ��ѹ��ָ��·����
	// eg : c:\test\222.zip -> c:\Windows\123
	static bool UnZip(LPCWSTR wzZipName, LPCWSTR wzDestPath);
};

#endif // ZIPTOOLS_H