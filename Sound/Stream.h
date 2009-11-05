/*
 Stream.h : A sound stream
*/

#pragma once

namespace NSound
{
	typedef unsigned long long StreamOffset;

	class CStream
	{
	public:
		CStream();
		virtual ~CStream();

		virtual unsigned char GetBitsPerSample() const=0;
		virtual unsigned char GetChannels() const=0;
		virtual unsigned long GetSampleRate() const=0;
		virtual unsigned long Read(void* Buffer, unsigned long Size)=0;
		virtual bool CanSeek() const;
		// Seeking?

	protected:
		virtual bool Lock();
		virtual bool Unlock();
	};

	class CTestStream : public CStream
	{
	public:
		CTestStream();
		virtual ~CTestStream();

		virtual unsigned char GetBitsPerSample() const;
		virtual unsigned char GetChannels() const;
		virtual unsigned long GetSampleRate() const;
		virtual unsigned long Read(void* Buffer, unsigned long Size);
	};
};
