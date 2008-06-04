// Version5Stream.cpp : UbiSoft version 3 and 5 audio stream decoding
//

#include "stdafx.h"
#include "Version5Stream.h"
#include "Adpcm.h"
#include "DataStream.h"

#define SwapWord(_Word) ((((unsigned short)(_Word)&0xFF00)>>8) | (((unsigned short)(_Word)&0x00FF)<<8))

CVersion5Stream::CVersion5Stream(CDataStream* Input) :
	CStreamHelper(Input),
	m_Type(5),
	m_NumberExtraSamples(0),
	m_Stereo(true),
	m_LeftSample(0),
	m_LeftIndex(0),
	m_RightSample(0),
	m_RightIndex(0)
{
	return;
}

CVersion5Stream::~CVersion5Stream()
{
	return;
}

bool CVersion5Stream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool CVersion5Stream::InitializeHeader(unsigned char Channels, unsigned char Force)
{
	// Check the parameters
	if(Channels<0 || Channels>2)
	{
		return false;
	}
	if(!(Force==0 || Force==3 || Force==5))
	{
		return false;
	}

	// Read the type from the file
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}
	m_InputStream->ExactRead(&m_Type, 1);
	if(Force)
	{
		m_Type=Force;
	}
	else
	{
		if(m_Type!=3 && m_Type!=5)
		{
			return false;
		}
	}

	// Read the rest of the first header
	m_InputStream->ExactIgnore(13);
	m_InputStream->ExactRead(&m_NumberExtraSamples, 2);
	m_InputStream->ExactRead(&m_LeftSample, 2);
	m_InputStream->ExactRead(&m_LeftIndex, 1);
	m_InputStream->ExactIgnore(1);
	m_InputStream->ExactRead(&m_RightSample, 2);
	m_InputStream->ExactRead(&m_RightIndex, 1);
	m_InputStream->ExactIgnore(5);

	// Theses are big-endian for some reason
	if(m_Type==3)
	{
		m_NumberExtraSamples=SwapWord(m_NumberExtraSamples);
		m_LeftSample=SwapWord(m_LeftSample);
		m_RightSample=SwapWord(m_RightSample);
	}

	// Figure out whether it is mono or stereo
	if(Channels==0)
	{
		// TODO: Scan through the data mono, then stereo and see which one makes sense
		std::cout << "Warning: Automatic channels detection has not yet been implemented. Assuming stereo." << std::endl;
		m_Stereo=true;
	}
	else if(Channels==1)
	{
		m_Stereo=false;
	}
	else if(Channels==2)
	{
		m_Stereo=true;
	}

	// Give a warning if the number of extra samples is unrecognized
	if(m_NumberExtraSamples!=10)
	{
		std::cerr << "Warning: The number of extra uncompressed samples is unrecognized (" << m_NumberExtraSamples << " samples)" << std::endl;
	}
	if(m_Stereo)
	{
		m_NumberExtraSamples*=2;
	}
	return true;
}

bool CVersion5Stream::DoDecodeBlock()
{
	// Process the uncompressed samples
	if(m_NumberExtraSamples)
	{
		// Prepare the buffers
		unsigned char* Buffer;
		unsigned long BufferLength=m_NumberExtraSamples*2;
		PrepareOutputBuffer(m_NumberExtraSamples);
		Buffer=(unsigned char*)m_InputStream->ExactRead(BufferLength);

		// Process the bytes
		for(unsigned long i=0;i<BufferLength;i+=2)
		{
			if(m_Type==3)
			{
				m_OutputBuffer[m_OutputBufferUsed]=SwapWord(Buffer[i]);
			}
			else
			{
				m_OutputBuffer[m_OutputBufferUsed]=Buffer[i];
			}
			m_OutputBufferUsed++;
			m_NumberExtraSamples--;
		}
		return true;
	}

	// Prepare the buffers
	unsigned char* Buffer;
	unsigned long BufferLength=RecommendBufferLength()/2;
	PrepareOutputBuffer(RecommendBufferLength());
	Buffer=(unsigned char*)m_InputStream->Read(BufferLength);

	// Calculate how many samples are needed
	m_OutputBufferUsed=BufferLength*2;
	if(m_OutputBufferUsed<1)
	{
		return true;
	}

	// Do the decompression
	if(!m_Stereo)
	{
		SAdpcmMonoParam Param;
		Param.InputBuffer=Buffer;
		Param.InputLength=BufferLength;
		Param.OutputBuffer=m_OutputBuffer;
		Param.FirstSample=m_LeftSample;
		Param.FirstIndex=m_LeftIndex;
		DecompressMonoAdpcm(&Param);
		m_LeftSample=Param.FirstSample;
		m_LeftIndex=Param.FirstIndex;
		m_RightSample=0;
		m_RightIndex=0;
	}
	else
	{
		SAdpcmStereoParam Param;
		Param.InputBuffer=Buffer;
		Param.InputLength=BufferLength;
		Param.OutputBuffer=m_OutputBuffer;
		Param.FirstLeftSample=m_LeftSample;
		Param.FirstLeftIndex=m_LeftIndex;
		Param.FirstRightSample=m_RightSample;
		Param.FirstRightIndex=m_RightIndex;
		DecompressStereoAdpcm(&Param);
		m_LeftSample=Param.FirstLeftSample;
		m_LeftIndex=Param.FirstLeftIndex;
		m_RightSample=Param.FirstRightSample;
		m_RightIndex=Param.FirstRightIndex;
	}
	return true;
}

unsigned long CVersion5Stream::GetSampleRate() const
{
	// Check each possible type
	if(m_Type==3)
	{
		return 36000;
	}
	else if(m_Type==5)
	{
		return 48000;
	}
	return 22050;
}

unsigned char CVersion5Stream::GetChannels() const
{
	return m_Stereo ? 2 : 1;
}

std::string CVersion5Stream::GetFormatName() const
{
	// Check each possible type
	if(m_Type==3)
	{
		return "ubichunk3";
	}
	else if(m_Type==5)
	{
		return "ubichunk5";
	}
	return "unknown";
}
