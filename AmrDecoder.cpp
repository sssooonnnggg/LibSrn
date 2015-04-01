#include "AmrDecoder.h"
#include <windows.h>
#include "DebugTools.h"
#include "FileTools.h"

#include <MMSystem.h>

extern "C" {
#include "interf_dec.h"
}

#pragma comment(lib, "Winmm.lib")

const int g_AmrSizeMap[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

AmrDecoder::AmrDecoder(const wchar_t* filePath)
	: m_filePath(filePath)
	, m_valid(true)
	, m_amrBuffer(NULL)
	, m_amrBufferSize(0)
	, m_wavBuffer(NULL)
	, m_wavBufferSize(0)
{
	ValidateFile();

	if ( !m_valid )
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] Invalid Amr File. [%s]\r\n", filePath);
}

AmrDecoder::~AmrDecoder()
{
	if ( m_amrBuffer )
		delete [] m_amrBuffer;

	if ( m_wavBuffer )
		HeapFree(GetProcessHeap(), NULL, m_wavBuffer);
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

bool AmrDecoder::ConvertToWav(const wchar_t* filePath)
{
	if ( !m_valid ) return false;

	ReadAmrBuffer();
	HANDLE hWav = CreateWavFile(filePath);

	if ( INVALID_HANDLE_VALUE == hWav )
		return false;

	DecodeBuffer();

	DWORD written = 0;
	WriteFile(hWav, m_wavBuffer, m_wavBufferSize, &written, NULL);

	WriteWavHeader(hWav, m_wavBufferSize);
	CloseHandle(hWav);

	return true;
}

void AmrDecoder::ReadAmrBuffer()
{
	if ( NULL != m_amrBuffer ) return;

	HANDLE hFile = CreateFile(
		m_filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD fileSize = GetFileSize(hFile, NULL);

	char header[6] = {0};
	DWORD readed = 0;

	ReadFile(hFile, header, 6, &readed, NULL);

	m_amrBufferSize = fileSize-6;
	m_amrBuffer = new unsigned char[m_amrBufferSize];
	ReadFile(hFile, m_amrBuffer, m_amrBufferSize, &readed, NULL);
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

void AmrDecoder::DecodeBuffer()
{
	int tick = GetTickCount();
	if ( m_wavBuffer )
		return;

	void* amr = Decoder_Interface_init();
	int pos = 0, dataLength = 0;

	int memorySize = 1024 * 1024;
	
	HANDLE heap = GetProcessHeap();
	int errId = GetLastError();

	m_wavBuffer = (unsigned char*)HeapAlloc(heap, NULL, memorySize);
	unsigned char* end = m_wavBuffer;

	while ( true ) 
	{
		if ( pos >= m_amrBufferSize )
			break;

		int size = g_AmrSizeMap[(m_amrBuffer[pos] >> 3) & 0x0F];
		short outbuffer[160] = {0};
		Decoder_Interface_Decode(amr, m_amrBuffer+pos, outbuffer, 0);

		unsigned char littleendian[320] = {0};
		unsigned char* ptr = littleendian;

		for ( int i = 0; i < 160; ++i ) 
		{
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}

		if ( dataLength + 320 > memorySize )
		{
			memorySize *= 2;
			m_wavBuffer = (unsigned char*)HeapReAlloc(heap, NULL, m_wavBuffer, memorySize);
			end = m_wavBuffer + dataLength;
		}

		CopyMemory(end, littleendian, 320);
		end += 320;

		pos = pos + 1 + size;
		dataLength += 320;
	}

	m_wavBufferSize = dataLength;
	Decoder_Interface_exit(amr);

	int delay = GetTickCount() - tick;
	DebugTools::OutputDebugPrintfW(L"[AmrDecoder] Delay: %d ms \r\n", delay);
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

void AmrDecoder::Load(const wchar_t* filePath)
{
	m_valid = true;

	if ( m_amrBuffer )
	{
		delete [] m_amrBuffer;
		m_amrBuffer = NULL;
		m_amrBufferSize = 0;
	}

	if ( m_wavBuffer )
	{
		HANDLE heap = GetProcessHeap();
		HeapFree(heap, NULL, m_wavBuffer);
		m_wavBuffer = NULL;
		m_wavBufferSize = 0;
	}

	m_filePath = filePath;

	ValidateFile();
}

bool AmrDecoder::GetWavHeader(WAVEFORMATEX& wavHeader)
{
	if ( !m_valid ) return false;

	WAVEFORMATEX header = {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16, 0};
	memcpy(&wavHeader, &header, sizeof(WAVEFORMATEX));

	return true;
}

bool AmrDecoder::GetWavBuffer(unsigned char* &buffer, int& size)
{
	if ( !m_valid ) return false;
	
	ReadAmrBuffer();
	DecodeBuffer();

	size = m_wavBufferSize;
	buffer = new unsigned char[size];

	CopyMemory(buffer, m_wavBuffer, size);

	return true;
}