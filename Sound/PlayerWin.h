/*
 PlayerWin.h : A Windows sound player
*/

#pragma once

#include "Sound/Player.h"

namespace NSound
{
	class CPlayerWin : public CPlayer
	{
	public:
		CPlayerWin();
		virtual ~CPlayerWin();

	protected:
		unsigned char m_BitsPerSample;
		unsigned char m_Channels;
		unsigned long m_SampleRate;
		HWAVEOUT m_WaveOut;

	protected:
		virtual bool DoOpenDevice(unsigned char BitsPerSample, unsigned char Channels, \
			unsigned long SampleRate);
		virtual bool DoCloseDevice();
		virtual bool DoPlay();
		virtual bool DoIsPlaying() const;
		virtual bool DoStop();
	};
};
