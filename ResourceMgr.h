/*!
	Description : Used To Add/Extract File To/From PE Resource Section.
	Authro		: Ruining Song
	Date		: 2013.10.28
	Remark		: 
			所有文件被嵌入到同一个资源 ID 下

			资源类型：RESOURCE_TYPE
			资源ID	：RESOURCE_ID
			资源结构：| 文件名 | 文件大小 | 文件内容 | 文件名 | 文件大小 | 文件名 |...

			注： 添加和释放不能同时使用
*/

#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <Windows.h>

class Buffer;

class ResourceMgr
{
public:

	// 构造函数
	// 参数： pe 文件路径，传入 NULL 表示自身
	ResourceMgr(LPCWSTR lpwzExePath);

	// 析构函数
	~ResourceMgr();

	// 添加文件
	BOOL AddFile(LPCWSTR lpwzFileName);

	// 打包，添加文件后执行
	BOOL Packet();

	// 释放文件
	// 参数： 要提取的文件名， 释放后的文件名
	BOOL Extract(LPCWSTR lpwzFileName);

	// 释放所有文件
	BOOL ExtractAll();

private:

	// 释放文件至 m_lpBuffer
	BOOL ExtractToBuffer();
	
	// ExtractToBuffer 的反操作
	BOOL AddBufferToResource();

private:
	static const DWORD RESOURCE_ID;				// 资源标识号
	static const LPCWSTR RESOURCE_TYPE;			// 资源类型
	WCHAR m_wzExePath[MAX_PATH];				// PE 文件路径
	Buffer* m_lpBuffer;							// 缓冲区
};

#endif // RESOURCEMGR_H