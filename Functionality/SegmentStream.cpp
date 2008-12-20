/*
 SegmentStream.cpp : A stream of concatenated segments
*/

#include "Pch.h"

#include "Gui/SegmentsListView.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"
#include "Decoding/AudioStream.h"
#include "Decoding/FileDataStream.h"
#include "Decoding/Version5Stream.h"
#include "Decoding/InterleavedStream.h"
#include "Decoding/OldInterleavedStream.h"
#include "Decoding/OggVorbisStream.h"
#include "Decoding/RawCompressedStream.h"
#include "Decoding/RawPcmStream.h"

// CSegmentStream Implementation
NDecFunc::CSegmentStream::CSegmentStream() :
	m_CurrentSegment(-1),
	m_InputStream(NULL),
	m_AudioStream(NULL)
{
	return;
}

NDecFunc::CSegmentStream::~CSegmentStream()
{
	Clear();
	ReleaseAudioSegment();
	return;
}

void NDecFunc::CSegmentStream::Add(const CSegment* Segment)
{
	if(Segment)
	{
		push_back(new CSegment(*Segment));
	}
	return;
}

NDecFunc::CSegment* NDecFunc::CSegmentStream::Get(unsigned long Index)
{
	if(Index>=GetCount())
	{
		return NULL;
	}
	return at(Index);
}

void NDecFunc::CSegmentStream::Clear()
{
	for(iterator Iter=begin();Iter!=end();++Iter)
	{
		delete *Iter;
		*Iter=NULL;
	}
	clear();
	m_CurrentSegment=-1;
	return;
}

unsigned long NDecFunc::CSegmentStream::GetCount() const
{
	return (unsigned long)size();
}

/*unsigned long NDecFunc::CSegmentStream::GetCurrent() const
{
	return m_CurrentSegment;
}

void NDecFunc::CSegmentStream::SetCurrent(unsigned long Index)
{
	if(Index==-1)
	{
		m_CurrentSegment=-1;
	}
	else if(Index<GetCount())
	{
		m_CurrentSegment=Index;
	}
	return;
}*/

void NDecFunc::CSegmentStream::Restart()
{
	ReleaseAudioSegment();
	m_CurrentSegment=-1;
	return;
}

NDecFunc::CSegment* NDecFunc::CSegmentStream::GetCurrentSegment() const
{
	unsigned long AudioSegment=m_CurrentSegment+1;

	// Make sure there are segments
	if(!GetCount())
	{
		return NULL;
	}

	// Get the next audio segment
	if(AudioSegment>=GetCount())
	{
		AudioSegment=0;
	}

	return at(AudioSegment);
}

unsigned long NDecFunc::CSegmentStream::GetProgess(unsigned long Max) const
{
	// Check for the easy ones
	if(m_CurrentSegment==-1)
	{
		return 0;
	}
	if(m_CurrentSegment==GetCount())
	{
		return Max;
	}
	
	// Get the amount processed by the input file
	unsigned long long Progress=m_CurrentSegment*Max/GetCount()+m_InputStream->Tell()*(Max/GetCount())/m_InputStream->GetLength();
	return Progress;
}

bool NDecFunc::CSegmentStream::Decode(short* Buffer, unsigned long& NumberSamples)
{
	// Check arguments
	if(NumberSamples==0)
	{
		return true;
	}
	if(!Buffer)
	{
		return false;
	}

	// Store some variables
	unsigned long NumberSamplesWanted=NumberSamples;
	NumberSamples=0;

	do
	{
		// Do we need to make a new one?
		if(!m_AudioStream || !m_InputFile)
		{
			while(true)
			{
				// Get the next audio segment
				m_CurrentSegment++;
				if(m_CurrentSegment>=GetCount())
				{
					// We're done
					return true;
				}

				// Initialize it
				if(!InitializeAudioSegment(*at(m_CurrentSegment)))
				{
					continue;
				}

				// Set the currently playing item in the segments list
				if(at(m_CurrentSegment)->GetListView())
				{
					at(m_CurrentSegment)->GetListView()->SetCurrentlyPlaying( \
						at(m_CurrentSegment)->GetListViewIndex());
				}
				break;
			}
		}

		// Try to decode some data
		unsigned long BufferWanted=NumberSamplesWanted-NumberSamples;
		try
		{
			if(!m_AudioStream->Decode(Buffer+NumberSamples, BufferWanted))
			{
				if(BufferWanted)
				{
					NumberSamples+=BufferWanted;
				}
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		catch(...)
		{
			if(BufferWanted)
			{
				NumberSamples+=BufferWanted;
			}
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
		NumberSamples+=BufferWanted;

		// Are we at the end of the stream?
		if(!BufferWanted)
		{
			ReleaseAudioSegment();
		}
	} while(NumberSamples<NumberSamplesWanted);
	return true;
}

// Template error handler to add in down there
/*catch(XDataException& e)
{
	//std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
	//std::cerr << e.GetMessage() << std::endl;
}
catch(XAudioException& e)
{
	//std::cerr << e.GetFriendlyMessage() << std::endl;
	//std::cerr << "Details: " << e.GetMessage() << std::endl;
}*/

bool NDecFunc::CSegmentStream::InitializeAudioSegment(CSegment& Segment)
{
	// Open the file
	m_InputFile.open(Segment.GetFilename().c_str(), std::ios_base::in | std::ios_base::binary);
	if(!m_InputFile.is_open())
	{
		// TODO: Better error handling
		return false;
	}

	// Create an audio input data stream
	m_InputStream=new CFileDataStream(&m_InputFile, Segment.GetOffset(), Segment.GetSize());

	// Create an audio output stream for each type
	if(Segment.GetType()==EUF_UBI_V3 || Segment.GetType()==EUF_UBI_V5)
	{
		CVersion5Stream* Stream=new CVersion5Stream(m_InputStream);
		m_AudioStream=Stream;

		// Try to initialize the header
		try
		{
			if(!Stream->InitializeHeader(Segment.GetSampleRate()))
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
	}
	else if(Segment.GetType()==EUF_UBI_IV8)
	{
		// Decode the stream
		CInterleavedStream* Stream=new CInterleavedStream(m_InputStream);
		m_AudioStream=Stream;

		// Try to initialize the header
		try
		{
			if(Segment.GetLayers().size()<1)
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
			Stream->SetCurrentLayers(Segment.GetLayers());

			if(!Stream->InitializeHeader(Segment.GetSampleRate(), Segment.GetChannels()))
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
	}
	else if(Segment.GetType()==EUF_UBI_IV2)
	{
		// Decode the stream
		COldInterleavedStream* Stream=new COldInterleavedStream(m_InputStream);
		m_AudioStream=Stream;

		// Try to initialize the header
		try
		{
			if(Segment.GetLayers().size()<1)
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
			Stream->SetCurrentLayers(Segment.GetLayers());

			if(!Stream->InitializeHeader(Segment.GetSampleRate()))
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
	}
	else if(Segment.GetType()==EUF_OGG)
	{
		COggVorbisStream* Stream=new COggVorbisStream(m_InputStream);
		m_AudioStream=Stream;

		// Try to initialize the header
		try
		{
			if(!Stream->InitializeHeader())
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
	}
	else if(Segment.GetType()==EUF_RAW)
	{
		// Decode the stream
		CRawPcmStream* Stream=new CRawPcmStream(m_InputStream);
		m_AudioStream=Stream;

		// Try to initialize the header
		try
		{
			if(!Stream->InitializeHeader(Segment.GetChannels()))
			{
				// TODO: Better error handling
				ReleaseAudioSegment();
				return false;
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			ReleaseAudioSegment();
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

void NDecFunc::CSegmentStream::ReleaseAudioSegment()
{
	delete m_AudioStream;
	m_AudioStream=NULL;
	delete m_InputStream;
	m_InputStream=NULL;
	if(m_InputFile.is_open())
	{
		m_InputFile.close();
	}
	return;
}
