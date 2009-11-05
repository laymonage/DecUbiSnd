/*
 Player.h : A sound player
*/

#pragma once

namespace NSound
{
	class CStream;

	class CPlayer
	{
	public:
		CPlayer();
		virtual ~CPlayer();

		virtual bool Play(CStream* Stream);
		virtual bool Stop();
		virtual bool IsPlaying() const;
		virtual bool CanSeek() const;
		// Seeking?
		virtual unsigned char GetBitsPerSample() const;
		virtual unsigned char GetChannels() const;
		virtual unsigned long GetSampleRate() const;

		static CPlayer* CreatePlayer();
		static void DestroyPlayer(CPlayer* Player);

	protected:
		CStream* m_Stream;

	protected:
		virtual unsigned long Read(void* Buffer, unsigned long Size);
		virtual bool DoOpenDevice(unsigned char BitsPerSample, unsigned char Channels, \
			unsigned long SampleRate)=0;
		virtual bool DoCloseDevice()=0;
		virtual bool DoPlay()=0;
		virtual bool DoIsPlaying() const=0;
		virtual bool DoStop()=0;
	};
};
