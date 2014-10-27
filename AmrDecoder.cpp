#include "AmrDecoder.h"
#include <windows.h>
#include "DebugTools.h"
#include "FileTools.h"

#include <MMSystem.h>

extern "C" {
#include "interf_dec.h"
}

#pragma comment(lib, "Winmm.lib")

const int g_Sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

AmrDecoder::AmrDecoder(const wchar_t* filePath)
	: m_filePath(filePath)
	, m_valid(true)
	, m_amrBuffer(NULL)
	, m_amrBufferSize(0)
	, m_wavBuffer(NULL)
	, m_wavBufferSize(0)
	, m_hWavOut(0)
{
	ValidateFile();

	if ( !m_valid )
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] Invalid Amr File. [%s]\r\n", filePath);
}

AmrDecoder::~AmrDecoder()
{
	if ( m_amrBuffer )
		delete [] m_amrBuffer;

	Stop();
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

	void* amr = Decoder_Interface_init();
	int pos = 0, dataLength = 0;

	while ( true ) 
	{
		if ( pos >= m_amrBufferSize )
			break;

		int size = g_Sizes[(m_amrBuffer[pos] >> 3) & 0x0F];
		short outbuffer[160] = {0};
		Decoder_Interface_Decode(amr, m_amrBuffer+pos, outbuffer, 0);

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

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	//printf("%x\r\n", uMsg);
}

bool AmrDecoder::Play()
{
	if ( !m_wavBuffer )
		if ( !ReadWavBuffer() ) return false;

	WAVEFORMATEX formatInfo = {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16, 0};

	MMRESULT result = waveOutOpen(&m_hWavOut, WAVE_MAPPER, &formatInfo, (DWORD_PTR)waveOutProc, NULL, CALLBACK_FUNCTION | WAVE_ALLOWSYNC);

	if ( MMSYSERR_NOERROR != result )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] waveOutOpen Failed.\r\n");
		return false;
	}


	WAVEHDR wavHeader = {m_wavBuffer, m_wavBufferSize, 0, 0, WHDR_BEGINLOOP | WHDR_ENDLOOP, 1, 0, 0};
	memcpy(&m_wavHeader, &wavHeader, sizeof(WAVEHDR));

	if ( MMSYSERR_NOERROR != waveOutPrepareHeader(m_hWavOut, &m_wavHeader, sizeof(WAVEHDR)) )
	{
		waveOutClose(m_hWavOut);
		m_hWavOut = NULL;
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] waveOutPrepareHeader Failed.\r\n");
		return false;
	}

	if ( MMSYSERR_NOERROR != waveOutWrite(m_hWavOut, &m_wavHeader, sizeof(WAVEHDR)) )
	{
		waveOutClose(m_hWavOut);
		m_hWavOut = NULL;
		DebugTools::OutputDebugPrintfW(L"[AmrDecoder] waveOutPrepareHeader Failed.\r\n");
		return false;
	}

	return true;
}

bool AmrDecoder::ReadWavBuffer()
{
	if ( m_wavBuffer )
		return true;

	WCHAR tempPath[MAX_PATH] = {0};
	GetTempPathW(MAX_PATH, tempPath);

	WCHAR tempWav[MAX_PATH] = {0};
	wsprintfW(tempWav, L"%stemp_%d.wav", tempPath, GetTickCount());

	if ( !ConvertToWav(tempWav) )
		return false;

	HANDLE hFile = CreateFileW(tempWav, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	m_wavBufferSize = GetFileSize(hFile, NULL) - 44;
	SetFilePointer(hFile, 40, NULL, FILE_BEGIN);
	int timeSec = m_wavBufferSize / 16000;

	m_wavBuffer = new char[m_wavBufferSize];

	DWORD readed = 0;
	ReadFile(hFile, m_wavBuffer, m_wavBufferSize, &readed, NULL);

	CloseHandle(hFile);
	DeleteFileW(tempWav);

	return true;
}

bool AmrDecoder::Stop()
{
	if ( m_hWavOut )
	{
		waveOutReset(m_hWavOut);
		waveOutClose(m_hWavOut);
		m_hWavOut = NULL;
		delete [] m_wavBuffer;
		m_wavBuffer = NULL;
		m_wavBufferSize = 0;
		return true;
	}

	return false;
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

	Stop();
	m_filePath = filePath;

	ValidateFile();
}