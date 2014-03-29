/*!
	Description : Useful File Utils.
	Author		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
*/

#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <Windows.h>

class FileTools
{
public:

	// �ж�ָ���ļ����ļ����Ƿ����
	static BOOL Exist(LPCWSTR lpwzFilePath);

	// ��ȡ�ļ�����Ŀ¼
	// eg: GetFileDir(L"C:\\Dir\\Test.exe", wzBuffer)
	// ִ��֮��wzBuffer : L"C:\\Dir"
	static VOID GetFileDir(IN LPCWSTR lpwzFile, OUT LPWSTR lpwzFilePath);

	// ��ȡ������������·��
	// ע : ����� buffer ����ӦΪ MAX_PATH
	static VOID GetExePath(OUT LPWSTR lpwzExePath);

	// ��ȡ�ļ���С���粻�����򷵻� -1
	static DWORD GetFileSize(LPCWSTR lpwzFile);

	// ��ȡ�ļ����ڴ�
	static BOOL ReadFileToMem(LPCWSTR lpwzFile, LPBYTE lpBuffer);

	// ���ڴ�������д�����ļ�
	static BOOL WriteFileFromMem(LPCWSTR lpwzFile, LPBYTE lpBuffer, DWORD dwLen);

	// ��·����ȡ�ļ���
	// eg: GetFileNameWithoutPath(L"C:\\Dir\\Test.exe", wzBuffer)
	// ִ��֮��: wzBuffer : L"Text.exe"
	static VOID GetFileNameWithoutPath(LPCWSTR lpwzPath, LPWSTR lpwzName);

	// �������Ŀ¼
	static VOID CreateDirectorys(LPCWSTR lpwzDir);
};

#endif // FILETOOLS_H