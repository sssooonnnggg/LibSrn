/*!
	Description : This class is used to play the .amr file format.
	Author		: Ruining Song
	Date		: 2014/10/28
*/

#ifndef AMRPLAYER_H
#define AMRPLAYER_H

#include <dsound.h>

class AmrDecoder;

class AmrPlayer
{
public:

	AmrPlayer(HWND window, const wchar_t* filePath);
	~AmrPlayer();

	bool Load(const wchar_t* filePath);

	bool Start();
	bool Stop();
	bool Pause();
	bool Resume();

	int GetDuration();

	bool SetPosition(double second);
	double GetCurrentPosition();

private:
	bool InitDSound();
	bool LockBuffer();

private:
	AmrDecoder* m_decoder;
	unsigned char* m_buffer;
	int m_bufferSize;
	bool m_valid;

	LPDIRECTSOUND m_ds;
	LPDIRECTSOUNDBUFFER m_dsBuffer;
	WAVEFORMATEX m_wavHeader;
	DSBUFFERDESC m_dsBufferDesc;

	HWND m_window;

	bool m_isPlaying;
};

#endif // AMRPLAYER_H