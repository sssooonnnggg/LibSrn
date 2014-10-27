#ifndef AMRDECODER_H
#define AMRDECODER_H

#include <Windows.h>

class AmrDecoder
{
public:
	AmrDecoder(const wchar_t* filePath);
	~AmrDecoder();

	/*!
	\brief
		Convert amr to wav.
	*/
	bool ConvertToWav(const wchar_t* filePath);

private:
	void ValidateFile();
	void ReadBuffer();
	HANDLE CreateWavFile(const wchar_t* filePath);
	void WriteWavHeader(HANDLE hWav, int dataLength);

private:
	const wchar_t* m_filePath;
	bool m_valid;
	unsigned char* m_buffer;
	int m_bufferSize;
};

#endif // AMRDECODER_H