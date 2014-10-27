#include "AmrDecoder.h"
#include <windows.h>
#include "DebugTools.h"
#include "FileTools.h"

extern "C" {
#include "interf_dec.h"
}

const int g_Sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

AmrDecoder::AmrDecoder(const wchar_t* filePath)
	: m_filePath(filePath)
	, m_valid(true)
	, m_buffer(NULL)
	, m_bufferSize(0)
{
	ValidateFile();

	if ( !m_valid )
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] Invalid Amr File. [%s]\r\n", filePath);
}

AmrDecoder::~AmrDecoder()
{
	if ( m_buffer )
		delete [] m_buffer;
}

void AmrDecoder::ValidateFile()
{
	if ( !FileTools::Exist(m_filePath) )
	{
		m_valid = false;
		return;
	}

	HANDLE hFile = CreateFile(
		m_filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		m_valid = false;
		return;
	}

	unsigned char header[7] = {0};
	DWORD readed = 0;

	if ( !ReadFile(hFile, header, 6, &readed, NULL) )
	{
		m_valid = false;
		CloseHandle(hFile);
		return;
	}

	if ( 0 != memcmp(header, "#!AMR\n", 6) )
		m_valid = false;

	CloseHandle(hFile);
}

void AmrDecoder::ReadBuffer()
{
	if ( NULL != m_buffer ) return;

	HANDLE hFile = CreateFile(
		m_filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD fileSize = GetFileSize(hFile, NULL);

	char header[6] = {0};
	DWORD readed = 0;

	ReadFile(hFile, header, 6, &readed, NULL);

	m_bufferSize = fileSize-6;
	m_buffer = new unsigned char[m_bufferSize];
	ReadFile(hFile, m_buffer, m_bufferSize, &readed, NULL);
}

bool AmrDecoder::ConvertToWav(const wchar_t* filePath)
{
	if ( !m_valid ) return false;

	ReadBuffer();
	HANDLE hWav = CreateWavFile(filePath);

	if ( INVALID_HANDLE_VALUE == hWav )
		return false;

	void* amr = Decoder_Interface_init();
	int pos = 0, dataLength = 0;

	while ( true ) 
	{
		if ( pos >= m_bufferSize )
			break;

		int size = g_Sizes[(m_buffer[pos] >> 3) & 0x0F];
		short outbuffer[160] = {0};
		Decoder_Interface_Decode(amr, m_buffer+pos, outbuffer, 0);

		unsigned char littleendian[320] = {0};
		unsigned char* ptr = littleendian;

		for ( int i = 0; i < 160; ++i ) 
		{
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}

		DWORD written = 0;
		WriteFile(hWav, littleendian, 320, &written, NULL);

		pos = pos + 1 + size;
		dataLength += 320;
	}

	Decoder_Interface_exit(amr);
	WriteWavHeader(hWav, dataLength);
	CloseHandle(hWav);

	return true;
}

HANDLE AmrDecoder::CreateWavFile(const wchar_t* filePath)
{
	HANDLE hWav = CreateFileW(filePath, FILE_GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ( INVALID_HANDLE_VALUE == hWav )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] Create File Failed. [%d] [%s] \r\n", GetLastError(), filePath);
		return hWav;
	}

	WriteWavHeader(hWav, 0);
	return hWav;
}

void AmrDecoder::WriteWavHeader(HANDLE hWav, int dataLength)
{
	DWORD written = 0;
	int dump = 0;
	short dump2 = 0;

	SetFilePointer(hWav, 0, 0, FILE_BEGIN);
	WriteFile(hWav, (LPCVOID)"RIFF", 4, &written, NULL);

	dump = 36 + dataLength;
	WriteFile(hWav, (LPCVOID)(&dump), 4, &written, NULL);

	WriteFile(hWav, (LPCVOID)"WAVE", 4, &written, NULL);
	WriteFile(hWav, (LPCVOID)"fmt ", 4, &written, NULL);

	dump = 16;
	WriteFile(hWav, (LPCVOID)(&dump), 4, &written, NULL);

	dump2 = 1;
	WriteFile(hWav, (LPCVOID)(&dump2), 2, &written, NULL);
	WriteFile(hWav, (LPCVOID)(&dump2), 2, &written, NULL);

	dump = 8000;
	WriteFile(hWav, (LPCVOID)(&dump), 4, &written, NULL);

	dump = 16000;
	WriteFile(hWav, (LPCVOID)(&dump), 4, &written, NULL);

	dump2 = 2;
	WriteFile(hWav, (LPCVOID)(&dump2), 2, &written, NULL);

	dump2 = 16;
	WriteFile(hWav, (LPCVOID)(&dump2), 2, &written, NULL);
	WriteFile(hWav, (LPCVOID)"data", 4, &written, NULL);
	WriteFile(hWav, (LPCVOID)(&dataLength), 4, &written, NULL);
}