// StreamHelper.cpp : Contains some helper functions for streams
//

#include "stdafx.h"
#include "StreamHelper.h"
#include "DataStream.h"

CStreamHelper::CStreamHelper(CDataStream* Input) :
	CAudioStream(Input),
	m_Initialized(false),
	m_OutputBuffer(NULL),
	m_OutputBufferLength(0),
	m_OutputBufferOffset(0),
	m_OutputBufferUsed(0)
{
	return;
}

CStreamHelper::~CStreamHelper()
{
	// This frees the buffer
	PrepareOutputBuffer(0);
	return;
}

void CStreamHelper::PrepareOutputBuffer(unsigned long OutputBufferLength)
{
	// Check the output buffer
	if(!m_OutputBuffer || m_OutputBufferLength<OutputBufferLength)
	{
		// Free the old one
		if(m_OutputBuffer)
		{
			delete[] m_OutputBuffer;
			m_OutputBuffer=NULL;
			m_OutputBufferLength=0;
		}

		// Allocate the new one
		if(OutputBufferLength)
		{
			m_OutputBuffer=new short[OutputBufferLength];
			m_OutputBufferLength=OutputBufferLength;
		}

		m_OutputBufferUsed=0;
		m_OutputBufferOffset=m_OutputBufferUsed;
	}
	return;
}

bool CStreamHelper::IsInitialized() const
{
	return m_Initialized;
}

bool CStreamHelper::Decode(short* Buffer, unsigned long& NumberSamples)
{
	// Check arguments
	if(!Buffer)
	{
		return false;
	}
	if(NumberSamples==0)
	{
		return true;
	}
	if(NumberSamples%GetChannels()!=0)
	{
		return false;
	}

	// Check to see if we are at the end of the stream
	if(!m_InputStream)
	{
		return false;
	}
	if(m_InputStream->IsEnd())
	{
		NumberSamples=0;
		return true;
	}

	// Some variables
	unsigned long BufferPos=0;
	unsigned long SamplesLeft=NumberSamples;

	while(true)
	{
		// See if there is any data in the buffer
		if(m_OutputBufferOffset<m_OutputBufferUsed)
		{
			// Calculate how much data is needed
			unsigned long SamplesToCopy=m_OutputBufferUsed-m_OutputBufferOffset;
			if(SamplesToCopy>SamplesLeft)
			{
				SamplesToCopy=SamplesLeft;
			}

			// Copy them
			for(unsigned long i=0;i<SamplesToCopy;i++)
			{
				Buffer[BufferPos]=m_OutputBuffer[m_OutputBufferOffset];
				m_OutputBufferOffset++;
				BufferPos++;
			}
			SamplesLeft-=SamplesToCopy;
		}

		// Was this enough?
		if(SamplesLeft<1)
		{
			break;
		}

		// Reset output
		m_OutputBufferUsed=0;
		m_OutputBufferOffset=0;

		// Check if it's the end of the stream
		if(m_InputStream->IsEnd())
		{
			break;
		}

		// Decode some data into the buffer
		if(!DoDecodeBlock())
		{
			PrepareOutputBuffer(0);
			return false;
		}

		// Exit if at the end of the stream
		if(m_OutputBufferUsed<1)
		{
			break;
		}
	};

	// Set the number of samples processed
	NumberSamples=BufferPos;
	return true;
}
