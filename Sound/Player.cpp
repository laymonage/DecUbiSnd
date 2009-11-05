/*
 Player.cpp : A sound player
*/

#include "Pch.h"

#include "Sound/Player.h"
#include "Sound/PlayerWin.h"
#include "Sound/Stream.h"

// CPlayer Implementation
NSound::CPlayer::CPlayer() :
	m_Stream(NULL)
{
	return;
}

NSound::CPlayer::~CPlayer()
{
	return;
}

bool NSound::CPlayer::Play(CStream* Stream)
{
	// Check arguments
	if(!Stream)
	{
		return false;
	}

	// Stop it if it's playing
	Stop();

	// Assign the new stream
	m_Stream=Stream;

	// Open the device
	if(!DoOpenDevice(m_Stream->GetBitsPerSample(), m_Stream->GetChannels(), \
		m_Stream->GetSampleRate()))
	{
		return false;
	}

	// Start playing
	if(!DoPlay())
	{
		DoCloseDevice();
		return false;
	}
	return true;
}

bool NSound::CPlayer::Stop()
{
	// Is it playing?
	if(DoIsPlaying())
	{
		if(!DoStop())
		{
			return false;
		}

		DoCloseDevice();
	}

	// Is there any stream?
	if(m_Stream)
	{
		m_Stream=NULL;
	}
	return true;
}

bool NSound::CPlayer::IsPlaying() const
{
	return m_Stream && DoIsPlaying();
}

bool NSound::CPlayer::CanSeek() const
{
	return false;
}

unsigned char NSound::CPlayer::GetBitsPerSample() const
{
	if(!m_Stream)
	{
		return 0;
	}
	return m_Stream->GetBitsPerSample();
}

unsigned char NSound::CPlayer::GetChannels() const
{
	if(!m_Stream)
	{
		return 0;
	}
	return m_Stream->GetChannels();
}

unsigned long NSound::CPlayer::GetSampleRate() const
{
	if(!m_Stream)
	{
		return 0;
	}
	return m_Stream->GetSampleRate();
}

NSound::CPlayer* NSound::CPlayer::CreatePlayer()
{
	return new CPlayerWin;
}

void NSound::CPlayer::DestroyPlayer(CPlayer* Player)
{
	delete Player;
	return;
}

unsigned long NSound::CPlayer::Read(void* Buffer, unsigned long Size)
{
	if(!m_Stream)
	{
		return 0;
	}
	return m_Stream->Read(Buffer, Size);
}
