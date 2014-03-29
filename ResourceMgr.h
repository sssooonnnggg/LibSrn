/*!
	Description : Used To Add/Extract File To/From PE Resource Section.
	Authro		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
			�����ļ���Ƕ�뵽ͬһ����Դ ID ��

			��Դ���ͣ�RESOURCE_TYPE
			��ԴID	��RESOURCE_ID
			��Դ�ṹ��| �ļ��� | �ļ���С | �ļ����� | �ļ��� | �ļ���С | �ļ��� |...

			ע�� ��Ӻ��ͷŲ���ͬʱʹ��
*/

#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <Windows.h>

class Buffer;

class ResourceMgr
{
public:

	// ���캯��
	// ������ pe �ļ�·�������� NULL ��ʾ����
	ResourceMgr(LPCWSTR lpwzExePath);

	// ��������
	~ResourceMgr();

	// ����ļ�
	BOOL AddFile(LPCWSTR lpwzFileName);

	// ���������ļ���ִ��
	BOOL Packet();

	// �ͷ��ļ�
	// ������ Ҫ��ȡ���ļ����� �ͷź���ļ���
	BOOL Extract(LPCWSTR lpwzFileName);

	// �ͷ������ļ�
	BOOL ExtractAll();

private:

	// �ͷ��ļ��� m_lpBuffer
	BOOL ExtractToBuffer();
	
	// ExtractToBuffer �ķ�����
	BOOL AddBufferToResource();

private:
	static const DWORD RESOURCE_ID;				// ��Դ��ʶ��
	static const LPCWSTR RESOURCE_TYPE;			// ��Դ����
	WCHAR m_wzExePath[MAX_PATH];				// PE �ļ�·��
	Buffer* m_lpBuffer;							// ������
};

#endif // RESOURCEMGR_H