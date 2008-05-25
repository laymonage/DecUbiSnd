// StreamHelper.cpp : Contains some helper functions for streams
//

#include "stdafx.h"
#include "StreamHelper.h"

CStreamHelper::CStreamHelper(std::istream& Input, std::streamsize Size) :
	CAudioStream(Input, Size),
	m_InputBuffer(NULL),
	m_OutputBuffer(NULL),
	m_InputBufferLength(0),
	m_OutputBufferLength(0),
	m_InputBufferOffset(0),
	m_OutputBufferOffset(0),
	m_InputBufferUsed(0),
	m_OutputBufferUsed(0)
{
	return;
}

CStreamHelper::CStreamHelper(std::istream& Input, std::streamoff Offset, std::streamsize Size) :
	CAudioStream(Input, Offset, Size),
	m_InputBuffer(NULL),
	m_OutputBuffer(NULL),
	m_InputBufferLength(0),
	m_OutputBufferLength(0),
	m_InputBufferOffset(0),
	m_OutputBufferOffset(0),
	m_InputBufferUsed(0),
	m_OutputBufferUsed(0)
{
	return;
}

CStreamHelper::~CStreamHelper()
{
	// This frees the buffers
	PrepareInputBuffer(0);
	PrepareOutputBuffer(0);
	return;
}

void CStreamHelper::PrepareInputBuffer(unsigned long InputBufferLength)
{
	// Check the input buffer
	if(!m_InputBuffer || m_InputBufferLength<InputBufferLength)
	{
		// Free the old one
		if(m_InputBuffer)
		{
			delete[] m_InputBuffer;
			m_InputBuffer=NULL;
			m_InputBufferLength=0;
		}

		// Allocate the new one
		if(InputBufferLength)
		{
			m_InputBuffer=new unsigned char[InputBufferLength];
			m_InputBufferLength=InputBufferLength;
		}

		m_InputBufferUsed=0;
		m_InputBufferOffset=m_InputBufferUsed;
	}
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

bool CStreamHelper::FillInputBuffer(unsigned long NumberBytes)
{
	// Check to see if the buffer can hold all of it
	if(NumberBytes>m_InputBufferLength)
	{
		return false;
	}

	// Reset the data in there already
	if(m_InputBufferUsed)
	{
		if(m_InputBufferOffset==m_InputBufferUsed)
		{
			m_InputBufferOffset=0;
			m_InputBufferUsed=0;
		}
		else if(m_InputBufferOffset>m_InputBufferUsed)
		{
			// Error!
		}
		else if(m_InputBufferOffset>0)
		{
			for(unsigned long i=0;i<m_InputBufferUsed-m_InputBufferOffset;i++)
			{
				m_InputBuffer[i]=m_InputBuffer[i+m_InputBufferOffset];
			}
			m_InputBufferUsed=m_InputBufferUsed-m_InputBufferOffset;
			m_InputBufferOffset=0;
		}
	}
	else
	{
		m_InputBufferOffset=0;
	}

	// Add more data if there is not enough already
	if(m_InputBufferUsed<NumberBytes)
	{
		// Calculate the number of bytes to read
		unsigned long BytesLeft=m_EndOffset-m_Input.tellg();
		unsigned long BytesToRead=NumberBytes-m_InputBufferUsed;
		if(BytesLeft<BytesToRead)
		{
			BytesToRead=BytesLeft;
		}

		// Read the buffer and update the variables
		m_Input.read((char*)(m_InputBuffer+m_InputBufferUsed), BytesToRead);
		m_InputBufferUsed+=BytesToRead;
	}
	return true;
}

unsigned long CStreamHelper::GetInputBytesLeft(unsigned long MaxNumberBytes)
{
	// Check if we went past the end
	if(m_EndOffset<m_Input.tellg())
	{
		return 0;
	}

	// Calculate the number of bytes to read
	unsigned long BytesLeft=m_EndOffset-m_Input.tellg();
	unsigned long BytesToRead=MaxNumberBytes;
	if(MaxNumberBytes!=0xFFFFFFFF && BytesLeft>MaxNumberBytes)
	{
		BytesLeft=MaxNumberBytes;
	}
	return BytesLeft;
}

bool CStreamHelper::Decode(short* Buffer, unsigned long& NumberSamples, unsigned long MaxInputBytes)
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

	// Check to make sure the file offset is sane
	if((std::streamoff)m_Input.tellg()==-1)
	{
		NumberSamples=0;
		return true;
	}
	else if(m_Input.tellg()<m_BeginOffset)
	{
		return false;
	}
	else if(m_Input.tellg()>m_EndOffset)
	{
		NumberSamples=0;
		return true;
	}

	// Some variables
	unsigned long SamplesLeft=NumberSamples;
	unsigned long BufferPos=0;
	unsigned long InputBytesLeft=m_EndOffset-m_Input.tellg();
	if(MaxInputBytes!=0xFFFFFFFF && InputBytesLeft>MaxInputBytes)
	{
		InputBytesLeft=MaxInputBytes;
	}

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

		// Decode some data into the buffer
		std::streamoff PrevOffset=m_Input.tellg();
		if(!DoDecodeBlock(InputBytesLeft))
		{
			PrepareInputBuffer(0);
			PrepareOutputBuffer(0);
			return false;
		}

		// Figure out how much input is left
		InputBytesLeft-=m_Input.tellg()-PrevOffset;

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
