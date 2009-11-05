/*
 Stream.cpp : A sound stream
*/

#include "Pch.h"

#include "Sound/Stream.h"

// CStream Implementation
NSound::CStream::CStream()
{
	return;
}

NSound::CStream::~CStream()
{
	return;
}

bool NSound::CStream::CanSeek() const
{
	return false;
}

bool NSound::CStream::Lock()
{
	// TODO: Implement me
	return true;
}

bool NSound::CStream::Unlock()
{
	// TODO: Implement me
	return true;
}


// Test stream
NSound::CTestStream::CTestStream()
{
	return;
}

NSound::CTestStream::~CTestStream()
{
	return;
}

unsigned char NSound::CTestStream::GetBitsPerSample() const
{
	return 16;
}

unsigned char NSound::CTestStream::GetChannels() const
{
	return 2;
}

unsigned long NSound::CTestStream::GetSampleRate() const
{
	return 44100;
}

unsigned long NSound::CTestStream::Read(void* Buffer, unsigned long Size)
{
	for(unsigned long i=0;i<Size;i++)
	{
		((unsigned char*)Buffer)[i]=rand()%256;
	}
	return Size;
}
