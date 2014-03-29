/*!
	Decription : Useful Network Utils.
	Author     : Ruining Song
	Date       : 2013.9.16
	Remark     :
*/

#ifndef NETTOOLS_H
#define NETTOOLS_H

#include <tchar.h>    
#include <windows.h>  
#include <setupapi.h>

#pragma comment (lib,"setupapi")

#define UnknownDevice TEXT("<Unknown Device>")

class NetTools
{
public:
	// ��������״̬ 1 - ���� 0 - ����
	static BOOL SetMacState(BOOL bState);
};

#endif // NETTOOLS_H