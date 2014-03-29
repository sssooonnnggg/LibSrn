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
	// …Ë÷√Õ¯ø®◊¥Ã¨ 1 - ∆Ù”√ 0 - Ω˚”√
	static BOOL SetMacState(BOOL bState);
};

#endif // NETTOOLS_H