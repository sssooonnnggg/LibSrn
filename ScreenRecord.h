/*!
	Description : Recording screen to an avi file using directshow.
	Author		: Ruining Song
	Date		: 2014/3/4
	Remark		: Depends on "pushsource" filter which located in 
	              "D:\WindowsSDK\Samples\multimedia\directshow\filters\pushsource"
*/

#ifndef SCREENRECORD_H
#define SCREENRECORD_H

#include <atlbase.h>
#include <windows.h>
#include <DShow.h>
#include <dshowasf.h>

class ScreenRecord
{
public:
	/*!
	\param
		Output file name
	\param
		Filter path(pushsource.dll)
	*/
	ScreenRecord(LPCWSTR wzAviName, LPCWSTR wzFilterPath);
	~ScreenRecord();

	void StartRecord();
	void StopRecord();

private:
	void InitConnection();

private:
	LPCWSTR m_wzAviName;
	IGraphBuilder* m_pGraph;
	IBaseFilter* m_pDeskTop;
	IBaseFilter* m_pAviMux;
	IBaseFilter* m_pFileWrite;
	IFileSinkFilter* m_pDes;
	IMediaControl* m_pControl;
	IWMWriter* m_pAsfFileWritter;
};

#endif // SCREENRECORD_H