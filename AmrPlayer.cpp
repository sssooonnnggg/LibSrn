#include "AmrPlayer.h"
#include "AmrDecoder.h"
#include "DebugTools.h"

#pragma comment(lib,"dsound.lib")

AmrPlayer::AmrPlayer(HWND window, const wchar_t* filePath)
	: m_decoder(new AmrDecoder(filePath))
	, m_buffer(NULL)
	, m_bufferSize(0)
	, m_valid(true)
	, m_ds(NULL)
	, m_dsBuffer(NULL)
	, m_window(window)
	, m_isPlaying(false)
{
	memset(&m_wavHeader, 0, sizeof(WAVEFORMATEX));
	memset(&m_dsBufferDesc, 0, sizeof(DSBUFFERDESC));

	if ( ! ( m_valid = m_decoder->GetWavBuffer(m_buffer, m_bufferSize) )
		|| ! ( m_valid = m_decoder->GetWavHeader(m_wavHeader) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Invalid amr file. [%s]\r\n", filePath);
		return;
	}

	if ( ! ( m_valid = InitDSound() ) )
		return;

	m_valid = LockBuffer();
}

AmrPlayer::~AmrPlayer()
{
	delete [] m_decoder;

	if ( m_buffer )
		delete [] m_buffer;

	if ( m_dsBuffer )
		m_dsBuffer->Release();

	if ( m_ds )
		m_ds->Release();
}

bool AmrPlayer::InitDSound()
{
	HRESULT hr = DirectSoundCreate(NULL, &m_ds, NULL);

	if ( DS_OK != hr )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] DirectSoundCreate Failed.[%d] \r\n", hr);
		return false;
	}

	if ( DS_OK != ( hr = m_ds->SetCooperativeLevel(m_window, DSSCL_NORMAL)) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] SetCooperativeLevel Failed.[%d] \r\n", hr);
		return false;
	}

	return true;
}

bool AmrPlayer::LockBuffer()
{
	LPVOID lockedMemory = NULL;
	DWORD lockedSize = 0;
	HRESULT hr;

	m_dsBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	m_dsBufferDesc.dwBufferBytes = m_bufferSize;
	m_dsBufferDesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
	m_dsBufferDesc.lpwfxFormat = &m_wavHeader;

	if ( FAILED( hr = m_ds->CreateSoundBuffer(&m_dsBufferDesc, &m_dsBuffer, NULL) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] CreateSoundBuffer Failed.[%d] \r\n", hr);
		return false;
	}

	if ( DS_OK != ( hr = m_dsBuffer->Lock(0, m_bufferSize, &lockedMemory, &lockedSize, NULL, NULL, 0) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Lock Failed.[%d] \r\n", hr);
		return false;
	}

	CopyMemory(lockedMemory, m_buffer, m_bufferSize);

	if ( DS_OK != ( hr = m_dsBuffer->Unlock(lockedMemory, lockedSize, NULL, 0) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Unlock Failed.[%d] \r\n", hr);
		return false;
	}

	return true;
}

bool AmrPlayer::Load(const wchar_t* filePath)
{
	if ( m_buffer )
	{
		delete [] m_buffer;
		m_buffer = NULL;
		m_bufferSize = NULL;
	}

	if ( !m_ds || !m_dsBuffer )
		return false;

	if ( m_isPlaying )
		Stop();

	m_decoder->Load(filePath);

	if ( ! ( m_valid = m_decoder->GetWavBuffer(m_buffer, m_bufferSize) )
		|| ! ( m_valid = m_decoder->GetWavHeader(m_wavHeader) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Invalid amr file. [%s]\r\n", filePath);
		return false;
	}

	return ( m_valid = LockBuffer() );
}

bool AmrPlayer::Start()
{
	if ( !m_valid || m_isPlaying ) 
		return false;

	HRESULT hr;

	if ( DS_OK != ( hr = m_dsBuffer->Play(0, 0, 0) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Play Sound Failed. [%d]\r\n", hr);
		return false;
	}

	m_isPlaying = true;

	return true;
}

bool AmrPlayer::Stop()
{
	if ( !m_valid || !m_isPlaying ) 
		return false;

	HRESULT hr;

	if ( DS_OK != ( hr = m_dsBuffer->Stop() ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] Stop Sound Failed. [%d]\r\n", hr);
		return false;
	}

	m_isPlaying = false;

	return true;
}

bool AmrPlayer::Pause()
{
	return Stop();
}

bool AmrPlayer::Resume()
{
	return Start();
}

int AmrPlayer::GetDuration()
{
	return m_bufferSize / m_wavHeader.nAvgBytesPerSec;
}

bool AmrPlayer::SetPosition( double second )
{
	if ( !m_valid ) return false;

	if ( second < 0
		|| second * m_wavHeader.nAvgBytesPerSec > m_bufferSize )
		return false;
	
	HRESULT hr;
	DWORD realPosition = second * m_wavHeader.nAvgBytesPerSec;

	if ( DS_OK != ( hr = m_dsBuffer->SetCurrentPosition(realPosition) ) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] SetCurrentPosition Failed. [%d]\r\n", hr);
		return false;
	}

	return true;
}

double AmrPlayer::GetCurrentPosition()
{
	if ( !m_valid ) return false;

	DWORD position = 0;
	HRESULT hr;

	if ( DS_OK != ( hr = m_dsBuffer->GetCurrentPosition(&position, NULL)) )
	{
		DebugTools::OutputDebugPrintfW(L"[AmrPlayer] GetCurrentPosition Failed. [%d]\r\n", hr);
		return -1;
	}

	return  ( (double)position ) / m_wavHeader.nAvgBytesPerSec;
}
