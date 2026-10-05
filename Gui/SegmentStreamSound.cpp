/*
 SegmentStreamSound.cpp : A segment stream sound
*/

#include "Pch.h"

#include "Gui/SegmentStreamSound.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"

// NDecGui Implementation
NDecGui::CSegmentStreamSound::CSegmentStreamSound(NDecFunc::CSegmentStream& Stream) :
	m_Stream(Stream),
	m_Looping(false),
	m_ExtraSamples(0),
	m_WaveOut(NULL),
	m_Playing(false),
	m_EndOfStream(false)
{
	memset(m_Headers, 0, sizeof(m_Headers));
	return;
}

NDecGui::CSegmentStreamSound::~CSegmentStreamSound()
{
	Stop();
	return;
}

void NDecGui::CSegmentStreamSound::SetLooping(bool Looping)
{
	m_Looping=Looping;
	return;
}

bool NDecGui::CSegmentStreamSound::IsLooping() const
{
	return m_Looping;
}

bool NDecGui::CSegmentStreamSound::IsStopped() const
{
	return !m_Playing;
}

bool NDecGui::CSegmentStreamSound::Play()
{
	Stop();

	// Get the first segment
	NDecFunc::CSegment* Segment=m_Stream.GetCurrentSegment();
	if(!Segment)
	{
		return false;
	}

	// Set up the sound format
	unsigned long SampleRate=Segment->GetSampleRate();
	unsigned long Channels=Segment->GetChannels();
	if(!SampleRate || !Channels)
	{
		return false;
	}
	WAVEFORMATEX Format;
	memset(&Format, 0, sizeof(Format));
	Format.wFormatTag=WAVE_FORMAT_PCM;
	Format.nChannels=(WORD)Channels;
	Format.nSamplesPerSec=SampleRate;
	Format.wBitsPerSample=16;
	Format.nBlockAlign=(WORD)(Format.nChannels*Format.wBitsPerSample/8);
	Format.nAvgBytesPerSec=Format.nSamplesPerSec*Format.nBlockAlign;
	if(waveOutOpen(&m_WaveOut, WAVE_MAPPER, &Format, 0, 0, CALLBACK_NULL)!=MMSYSERR_NOERROR)
	{
		m_WaveOut=NULL;
		return false;
	}

	// One second of silence is appended at the end
	m_ExtraSamples=SampleRate*Channels;
	m_EndOfStream=false;

	// Prepare and queue the buffers
	unsigned long BufferSamples=SampleRate*BufferMilliseconds/1000*Channels;
	for(unsigned long i=0;i<NumberBuffers;i++)
	{
		m_Buffers[i].assign(BufferSamples, 0);
		memset(&m_Headers[i], 0, sizeof(WAVEHDR));
		m_Headers[i].lpData=(LPSTR)&m_Buffers[i][0];
		m_Headers[i].dwBufferLength=BufferSamples*2;
		if(waveOutPrepareHeader(m_WaveOut, &m_Headers[i], sizeof(WAVEHDR))!=MMSYSERR_NOERROR)
		{
			m_Headers[i].dwFlags=0;
			Close();
			return false;
		}
	}
	m_Playing=true;
	waveOutPause(m_WaveOut);
	for(unsigned long i=0;i<NumberBuffers;i++)
	{
		Fill(i);
	}
	waveOutRestart(m_WaveOut);
	if(!m_Playing)
	{
		Close();
		return false;
	}

	wxTimer::Start(BufferMilliseconds/2);
	return true;
}

void NDecGui::CSegmentStreamSound::Stop()
{
	wxTimer::Stop();
	Close();
	return;
}

void NDecGui::CSegmentStreamSound::Close()
{
	m_Playing=false;
	if(m_WaveOut)
	{
		waveOutReset(m_WaveOut);
		for(unsigned long i=0;i<NumberBuffers;i++)
		{
			if(m_Headers[i].dwFlags & WHDR_PREPARED)
			{
				waveOutUnprepareHeader(m_WaveOut, &m_Headers[i], sizeof(WAVEHDR));
			}
		}
		waveOutClose(m_WaveOut);
		m_WaveOut=NULL;
	}
	memset(m_Headers, 0, sizeof(m_Headers));
	return;
}

// Queue a buffer, returns false if the stream is finished
bool NDecGui::CSegmentStreamSound::Fill(unsigned long Index)
{
	if(m_EndOfStream)
	{
		return false;
	}
	unsigned long Samples=GetData(&m_Buffers[Index][0], (unsigned long)m_Buffers[Index].size());
	if(!Samples)
	{
		m_EndOfStream=true;
		return false;
	}
	m_Headers[Index].dwBufferLength=Samples*2;
	m_Headers[Index].dwFlags&=~WHDR_DONE;
	if(waveOutWrite(m_WaveOut, &m_Headers[Index], sizeof(WAVEHDR))!=MMSYSERR_NOERROR)
	{
		m_EndOfStream=true;
		return false;
	}
	return true;
}

void NDecGui::CSegmentStreamSound::Notify()
{
	if(!m_Playing)
	{
		return;
	}

	// Refill finished buffers, and see if anything is still queued
	bool Queued=false;
	for(unsigned long i=0;i<NumberBuffers;i++)
	{
		if(m_Headers[i].dwFlags & WHDR_DONE)
		{
			if(!Fill(i))
			{
				m_Headers[i].dwFlags&=~WHDR_DONE;
			}
		}
		if((m_Headers[i].dwFlags & WHDR_INQUEUE) && !(m_Headers[i].dwFlags & WHDR_DONE))
		{
			Queued=true;
		}
	}

	if(!Queued)
	{
		// Everything has been played
		wxTimer::Stop();
		Close();
	}
	return;
}

unsigned long NDecGui::CSegmentStreamSound::GetData(short* Buffer, unsigned long NumberSamples)
{
	// Decode some samples
	unsigned long SamplesDecoded=NumberSamples;
	if(!m_Stream.Decode(Buffer, SamplesDecoded))
	{
		return SamplesDecoded;
	}

	// If we are looping, look for more
	if(m_Looping && SamplesDecoded<NumberSamples)
	{
		unsigned long NewDecoded=NumberSamples-SamplesDecoded;
		m_Stream.Restart();
		if(!m_Stream.Decode(Buffer+SamplesDecoded, NewDecoded))
		{
			return SamplesDecoded;
		}
		SamplesDecoded+=NewDecoded;
	}

	// Check to see if we need to add any extra samples
	if(SamplesDecoded<NumberSamples && m_ExtraSamples)
	{
		unsigned long Extra=NumberSamples-SamplesDecoded;
		if(Extra>m_ExtraSamples)
		{
			Extra=m_ExtraSamples;
		}
		memset(Buffer+SamplesDecoded, 0, Extra*2);
		SamplesDecoded+=Extra;
		m_ExtraSamples-=Extra;
	}
	return SamplesDecoded;
}
