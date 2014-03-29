
#include "stdafx.h"

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#define ZLIB_WINAPI
#define WRITEBUFFERSIZE (16384)

#include "ZipTools.h"

#include "DebugTools.h"
#include "FileTools.h"

#include "ioapi.h"
#include "zip.h"
#include "unzip.h"
#include "iowin32.h"

#pragma comment(lib, "zlibstat.lib")

// ////////////////////////////////////////////////////////////////////////////////
// @global 得到相对文件名
//
void GetRelativeFileName(LPCWSTR wzFullName, LPCWSTR wzParentPath, LPWSTR wzRelativeName)
{
	LPCWSTR wzMid = wzFullName + wcslen(wzParentPath);
	wcscpy_s(wzRelativeName, MAX_PATH, wzMid);
}

// ////////////////////////////////////////////////////////////////////////////////
// @global 将文件写入至 zip
//
void WriteFileToZip(WIN32_FIND_DATAW ffd, zipFile& zf, LPCWSTR wzFullPath, LPCWSTR wzParentPath, PBYTE buf)
{
	FILE * fin;
	zip_fileinfo zi = {0};
	FILETIME ftLocal;
	FileTimeToLocalFileTime(&(ffd.ftLastWriteTime),&ftLocal);
	FileTimeToDosDateTime(&ftLocal,((LPWORD)&zi.dosDate)+1,((LPWORD)&zi.dosDate)+0);

	WCHAR wzRelativeName[MAX_PATH] = {0};
	GetRelativeFileName(wzFullPath, wzParentPath, wzRelativeName);

	CHAR szRelativeName[1024] = {0};
	WideCharToMultiByte(CP_ACP, 0, wzRelativeName, -1, szRelativeName, 1024, NULL, NULL);
	int err = zipOpenNewFileInZip3_64(
		zf,
		szRelativeName,
		&zi,
		NULL,0,NULL,0,NULL,
		Z_DEFLATED,
		9,0,
		-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
		NULL, 0, 0);

	if ( ZIP_OK != err )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [AddFileToZip] Error In Opening File [%s] \r\n", wzRelativeName);
	}

	fin = _wfopen(wzRelativeName, L"rb");
	if ( NULL == fin )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [AddFileToZip] Error In Opening %s For Reading.\r\n", wzRelativeName);
	}

	unsigned long size_read = 0;
	unsigned long size_buf = WRITEBUFFERSIZE;

	do
	{
		err = ZIP_OK;
		size_read = (int)fread(buf,1,size_buf,fin);

		if ( size_read > 0 )
		{
			err = zipWriteInFileInZip (zf,buf,size_read);

			if ( err < 0 )
			{
				DebugTools::OutputDebugPrintf(
					L"[ZipTools] [AddFileToZip] Error In Writing %s In the Zipfile.\r\n",
					wzRelativeName);
			}
		}

	} while ((err == ZIP_OK) && (size_read>0));

	fclose(fin);
	zipCloseFileInZip(zf);
}

// ////////////////////////////////////////////////////////////////////////////////
// @global 将文件夹内文件添加进 zip ( 递归 )
//
void AddFileToZip(zipFile& zf, LPCWSTR wzPath, LPCWSTR wzParentPath, PBYTE buf)
{
	WIN32_FIND_DATAW ffd = {0};
	WCHAR wzFindStr[MAX_PATH] = {0};
	
	wsprintf(wzFindStr, L"%s\\*", wzPath);
	HANDLE hFind = FindFirstFileW(wzFindStr, &ffd);

	do 
	{
		if ( 0 == wcscmp(ffd.cFileName, L".")
			|| 0 == wcscmp(ffd.cFileName, L"..") )
		{
			continue;
		}

		if ( ffd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY )
		{
			WCHAR wzSubDir[MAX_PATH] = {0};
			wsprintf(wzSubDir, L"%s\\%s", wzPath, ffd.cFileName);
			AddFileToZip(zf, wzSubDir, wzParentPath, buf);
			continue;
		}

		WCHAR wzFullPath[MAX_PATH] = {0};
		wsprintf(wzFullPath, L"%s\\%s", wzPath, ffd.cFileName);
		WriteFileToZip(ffd, zf, wzFullPath, wzParentPath, buf);

		DebugTools::OutputDebugPrintf(L"[ZipTools] [AddFileToZip] Add File Success. [%s]\r\n", wzFullPath);

	} while ( 0 != FindNextFileW(hFind, &ffd));
}

// ////////////////////////////////////////////////////////////////////////////////
// @global 修改文件时间
//
void ChangeFileTime(const char *filename, unsigned long dosdate, tm_unz tmu_date)
{
	HANDLE hFile;
	FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

	hFile = CreateFileA(filename,GENERIC_READ | GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,0,NULL);
	GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
	DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
	LocalFileTimeToFileTime(&ftLocal,&ftm);
	SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
	CloseHandle(hFile);
}

// ////////////////////////////////////////////////////////////////////////////////
// @global 释放单个文件
//
int ExtractSingleFile(unzFile& uf)
{
	char filename_inzip[256] = {0};
	unz_file_info64 file_info;

	int err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if ( UNZ_OK != err )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [ExtractSingleFile] unzGetCurrentFileInfo64 Failed.[%d]\r\n", err);
		return err;
	}

	unsigned int size_buf = WRITEBUFFERSIZE;
	PBYTE buf = new BYTE[size_buf];

	// 判断名字是否以 '\\'　结尾，如果以 '\\'
	// 结尾，代表这是一个目录名，创建目录，
	// 反之则创建文件
	if ( '\\' == filename_inzip[strlen(filename_inzip)-1] )
	{
		WCHAR wzDirName[1024] = {0};
		MultiByteToWideChar(CP_ACP, 0, filename_inzip, -1, wzDirName, 1024);
		FileTools::CreateDirectorys(wzDirName);
	}
	else
	{
		err = unzOpenCurrentFilePassword(uf, NULL);

		if ( err != UNZ_OK)
		{
			DebugTools::OutputDebugPrintf(
				L"[ZipTools] [ExtractSingleFile] unzOpenCurrentFilePassword Failed.[%d]\r\n", err);
		}

		// 创建文件所在的目录
		WCHAR wzFileName[MAX_PATH] = {0};
		MultiByteToWideChar(CP_ACP, 0, filename_inzip, -1, wzFileName, MAX_PATH);
		WCHAR wzDirName[MAX_PATH] = {0};
		FileTools::GetFileDir(wzFileName, wzDirName);
		FileTools::CreateDirectorys(wzDirName);

		DebugTools::OutputDebugPrintf(
			L"[FileTools] [UnZip] Extracting %s ...\r\n", wzFileName);
		
		FILE* fout = fopen64(filename_inzip, "wb");

		do
		{
			err = unzReadCurrentFile(uf,buf,size_buf);

			if ( err < 0 )
			{
				DebugTools::OutputDebugPrintf(
					L"[ZipTools] [ExtractSingleFile] unzReadCurrentFile Failed.[%d]\r\n", err);
				break;
			}
			if ( err > 0 )
			{
				if ( fwrite(buf,err,1,fout) != 1 )
				{
					DebugTools::OutputDebugPrintf(
						L"[ZipTools] [ExtractSingleFile] Write File Failed.[%s]\r\n", 
						filename_inzip);

					err = UNZ_ERRNO;
					break;
				}
			}
		} while ( err > 0 );

		if ( fout )
		{
			fclose(fout);
		}

		// 修改文件时间
		ChangeFileTime(filename_inzip, file_info.dosDate, file_info.tmu_date);

		unzCloseCurrentFile(uf);
		delete [] buf;
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// @global 释放文件
//
void ExtractFile(unzFile& uf)
{
	unz_global_info64 gi;

	int err = unzGetGlobalInfo64(uf, &gi);

	if ( UNZ_OK != err )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [ExtractFile] unzGetGlobalInfo64 Failed.\r\n");
	}

	for (int i = 0 ; i < gi.number_entry; i++)
	{
		if ( ExtractSingleFile(uf) != UNZ_OK )
		{
			break;
		}

		if ( i + 1 < gi.number_entry )
		{
			err = unzGoToNextFile(uf);
			if ( err != UNZ_OK )
			{
				DebugTools::OutputDebugPrintf(
					L"[ZipTools] [ExtractFile] unzGoToNextFile Failed.\r\n");
				break;
			}
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////////
// @public @static 将指定文件名打包成指定 zip
//
bool ZipTools::Zip(LPCWSTR wzDirPath, LPCWSTR wzDestName)
{
	//
	// 检查指定路径是否存在
	//
	if ( !FileTools::Exist(wzDirPath) )
	{
		DebugTools::OutputDebugPrintf(L"[ZipTools] [Zip] Path Not Exist. [%s]\r\n", wzDirPath);
		return false;
	}

	WCHAR wzDestDir[MAX_PATH] = {0};
	FileTools::GetFileDir(wzDestName, wzDestDir);

	if ( !FileTools::Exist(wzDestDir) )
	{
		DebugTools::OutputDebugPrintf(L"[ZipTools] [Zip] Path Not Exist. [%s]\r\n", wzDestDir);
		return false;
	}

	// 设置当前工作目录为 wzDirPath 的上一层
	WCHAR wzParentDir[MAX_PATH] = {0};
	FileTools::GetFileDir(wzDirPath, wzParentDir);
	wcscat_s(wzParentDir, L"\\");
	SetCurrentDirectoryW(wzParentDir);

	//
	// 创建 zip 文件
	//
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64W(&ffunc);
	zipFile zf; zf = zipOpen2_64(wzDestName, 0, NULL, &ffunc);

	if ( NULL == zf )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [Zip] Create Zip File Failed. [%s] \r\n", wzDestName);
		return false;
	}

	unsigned long size_read = 0;
	unsigned long size_buf = WRITEBUFFERSIZE;
	PBYTE buf = new BYTE [size_buf];

	//
	// 将文件夹下所有文件添加进 zip
	//
	AddFileToZip(zf, wzDirPath, wzParentDir, buf);

	zipClose(zf,NULL);

	delete [] buf;

	return true;
}

// ////////////////////////////////////////////////////////////////////////////////
// @public @static 将指定 zip 解压至指定路径下
//
bool ZipTools::UnZip(LPCWSTR wzZipName, LPCWSTR wzDestPath)
{
	//
	// 检查指定文件和文件夹是否存在
	//
	if ( !FileTools::Exist(wzZipName) )
	{
		DebugTools::OutputDebugPrintf(L"[ZipTools] [UnZip] File Not Exist. [%s]\r\n", wzZipName);
		return false;
	}

	if ( !FileTools::Exist(wzDestPath) )
	{
		DebugTools::OutputDebugPrintf(L"[ZipTools] [Zip] Path Not Exist. [%s]\r\n", wzDestPath);
		return false;
	}

	// 打开 zip 文件
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64W(&ffunc);
	unzFile uf = unzOpen2_64(wzZipName, &ffunc);

	if ( NULL == uf )
	{
		DebugTools::OutputDebugPrintf(
			L"[ZipTools] [Zip] Open Zip File Failed.[%s]\r\n", wzZipName);
	}

	// 设置指定目录为工作目录
	SetCurrentDirectoryW(wzDestPath);

	// 释放文件
	ExtractFile(uf);

	unzClose(uf);

	return true;
}