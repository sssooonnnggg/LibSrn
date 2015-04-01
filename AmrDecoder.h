/*!
	Description : This class is used to decode the .amr file format.
	Author		: Ruining Song
	Date		: 2014/10/28
*/

#ifndef AMRDECODER_H
#define AMRDECODER_H

#include <Windows.h>

class AmrDecoder
{
public:

	AmrDecoder(const wchar_t* filePath);
	~AmrDecoder();

	bool ConvertToWav(const wchar_t* filePath);
	void Load(const wchar_t* filePath);

	/*!
	\brief
		Allocated by AmrDecoder.
	*/

	bool GetWavHeader(WAVEFORMATEX& wavHeader);
	bool GetWavBuffer(unsigned char* &buffer, int& size);

private:
	void ValidateFile();

	void ReadAmrBuffer();
	void DecodeBuffer();

	HANDLE CreateWavFile(const wchar_t* filePath);
	void WriteWavHeader(HANDLE hWav, int dataLength);

private:
	const wchar_t* m_filePath;
	bool m_valid;

	unsigned char* m_amrBuffer;
	int m_amrBufferSize;

	unsigned char* m_wavBuffer;
	int m_wavBufferSize;
};

#endif // AMRDECODER_H