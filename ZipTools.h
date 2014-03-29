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

	// 将指定文件夹打包成指定 zip
	// eg : C:\Windows\123 -> D:\test\222.zip;
	static bool Zip(LPCWSTR wzDirPath, LPCWSTR wzDestName);

	// 将指定 zip 解压至指定路径下
	// eg : c:\test\222.zip -> c:\Windows\123
	static bool UnZip(LPCWSTR wzZipName, LPCWSTR wzDestPath);
};

#endif // ZIPTOOLS_H