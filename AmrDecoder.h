#ifndef AMRDECODER_H
#define AMRDECODER_H

#include <Windows.h>
#include <MMSystem.h>

class AmrDecoder
{
public:
	AmrDecoder(const wchar_t* filePath);
	~AmrDecoder();

	bool ConvertToWav(const wchar_t* filePath);

	bool Play();
	bool Stop();

	void Load(const wchar_t* filePath);

private:
	void ValidateFile();
	void ReadAmrBuffer();
	bool ReadWavBuffer();
	HANDLE CreateWavFile(const wchar_t* filePath);
	void WriteWavHeader(HANDLE hWav, int dataLength);

private:
	const wchar_t* m_filePath;
	bool m_valid;
	unsigned char* m_amrBuffer;
	int m_amrBufferSize;
	char* m_wavBuffer;
	int m_wavBufferSize;
	HWAVEOUT m_hWavOut;
	WAVEHDR m_wavHeader;			// Ensure the wave header is kept when sound is playing.
};

#endif // AMRDECODER_H