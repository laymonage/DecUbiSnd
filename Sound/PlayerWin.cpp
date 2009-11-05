/*
 PlayerWin.cpp : A Windows sound player
*/

#include "Pch.h"

#include <windows.h>
#include <mmsystem.h>

#include "Sound/PlayerWin.h"

// CPlayerWin Implementation
NSound::CPlayerWin::CPlayerWin()
{
	return;
}

NSound::CPlayerWin::~CPlayerWin()
{
	return;
}

bool NSound::CPlayerWin::DoOpenDevice(unsigned char BitsPerSample, \
									  unsigned char Channels, \
									  unsigned long SampleRate)
{
	// Set some variables
	m_BitsPerSample=BitsPerSample;
	m_Channels=Channels;
	m_SampleRate=SampleRate;

	// Initialize the sound format
	WAVEFORMATEX SndFmt;
	memset(&SndFmt, 0, sizeof(WAVEFORMATEX));
	SndFmt.wFormatTag=WAVE_FORMAT_PCM;
	SndFmt.nChannels=m_Channels;
	SndFmt.nSamplesPerSec=m_SampleRate;
	SndFmt.wBitsPerSample=m_BitsPerSample;
	SndFmt.nBlockAlign=(SndFmt.nChannels*SndFmt.wBitsPerSample)/8;
	SndFmt.nAvgBytesPerSec=SndFmt.nSamplesPerSec*SndFmt.nBlockAlign;
/*	if(waveOutOpen(&m_WaveOut, WAVE_MAPPER, &SndFmt, (unsigned long)waveOutProc, (DWORD)this,\
	CALLBACK_FUNCTION)!=0) {
		return false;
	}

	// Allocate the buffer
	if((hSnd->SndBuf1=(BYTE*)malloc(BUFFERSIZE))==0) {
		sndSetErrorCode(hSnd, 5);
		return FALSE;
	}
	hSnd->WHDR1.lpData=hSnd->SndBuf1;
	hSnd->WHDR1.dwBufferLength=BUFFERSIZE;
	hSnd->WHDR1.dwLoops=1;
	hSnd->WHDR1.dwFlags=WHDR_BEGINLOOP;
	if(waveOutPrepareHeader(hSnd->hWO, &hSnd->WHDR1, sizeof(WAVEHDR))!=0) {
		sndSetErrorCode(hSnd, 3);
		return FALSE;
	}
	if((hSnd->SndBuf2=(BYTE*)malloc(BUFFERSIZE))==0) {
		sndSetErrorCode(hSnd, 6);
		return FALSE;
	}
	hSnd->WHDR2.lpData=hSnd->SndBuf2;
	hSnd->WHDR2.dwBufferLength=BUFFERSIZE;
	hSnd->WHDR2.dwLoops=1;
	hSnd->WHDR2.dwFlags=WHDR_ENDLOOP;
	if(waveOutPrepareHeader(hSnd->hWO, &hSnd->WHDR2, sizeof(WAVEHDR))!=0) {
		sndSetErrorCode(hSnd, 4);
		return FALSE;
	}*/
	return true;
}

bool NSound::CPlayerWin::DoCloseDevice()
{
	return true;
}

bool NSound::CPlayerWin::DoPlay()
{
	return true;
}

bool NSound::CPlayerWin::DoIsPlaying() const
{
	return true;
}

bool NSound::CPlayerWin::DoStop()
{
	return true;
}
