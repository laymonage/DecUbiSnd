// Version5Stream.cpp : UbiSoft version 3 and 5 audio stream decoding
//

#include "stdafx.h"
#include "Version5Stream.h"
#include "Adpcm.h"

CVersion5Stream::CVersion5Stream(std::istream& Input, std::streamsize Size) :
	CAudioStream(Input, Size),
	m_Type(5),
	m_Stereo(true),
	m_LeftSample(0),
	m_LeftIndex(0),
	m_RightSample(0),
	m_RightIndex(0)
{
	return;
}

CVersion5Stream::CVersion5Stream(std::istream& Input, std::streamoff Offset, std::streamsize Size) :
	CAudioStream(Input, Offset, Size),
	m_Type(5),
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

bool CVersion5Stream::InitHeader(unsigned char Channels, unsigned char Force)
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

	// Check the input
	if((m_EndOffset-m_BeginOffset)<100)
	{
		return false;
	}

	// Read the type from the file
	m_Input.seekg(m_BeginOffset);
	m_Type=m_Input.get();
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
	m_Input.seekg(15, std::ios_base::cur);
	m_Input.read((char*)&m_LeftSample, 2);
	m_Input.read((char*)&m_LeftIndex, 1);
	m_Input.seekg(1, std::ios_base::cur);
	m_Input.read((char*)&m_RightSample, 2);
	m_Input.read((char*)&m_RightIndex, 1);
	m_Input.seekg(5, std::ios_base::cur);

	m_LeftSample=0; // ?????????
	m_RightSample=0; // ?????????

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

	// Scan through the second header
	if(!m_Stereo)
	{
		// Perhaps this should not be hardcoded, but rather corresponds to something
		// in the header?
		m_Input.seekg(20, std::ios_base::cur);
	}
	else
	{
		m_Input.seekg(40, std::ios_base::cur);
	}
	return true;
}

bool CVersion5Stream::Decode(short* Buffer, unsigned long& NumberSamples)
{
	// Check arguments
	if(!Buffer)
	{
		return false;
	}
	if(NumberSamples==0 || NumberSamples%2!=0)
	{
		return false;
	}
	// Check to make sure the file offset is sane
	if(m_Input.tellg()<m_BeginOffset)
	{
		return false;
	}
	if(m_Input.tellg()>=m_EndOffset)
	{
		NumberSamples=0;
		return true;
	}

	// Calculate how many samples are needed
	unsigned long SamplesLeft=(m_EndOffset-m_Input.tellg())*2;
	unsigned long BytesLeft;
	if(SamplesLeft<NumberSamples)
	{
		NumberSamples=SamplesLeft;
	}
	BytesLeft=NumberSamples/2;

	// Allocate a buffer and read into the file
	unsigned char* InputBuffer=new unsigned char[BytesLeft];
	m_Input.read((char*)InputBuffer, BytesLeft);

	// Do the decompression
	if(!m_Stereo)
	{
		SAdpcmMonoParam Param;
		Param.InputBuffer=InputBuffer;
		Param.InputLength=BytesLeft;
		Param.OutputBuffer=Buffer;
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
		Param.InputBuffer=InputBuffer;
		Param.InputLength=BytesLeft;
		Param.OutputBuffer=Buffer;
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

	// Clean up
	delete [] InputBuffer;
	return true;
}

unsigned long CVersion5Stream::GetSampleRate()
{
	// Check each possible type
	if(m_Type==3)
	{
		return 32000;
	}
	else if(m_Type==5)
	{
		return 44100;
	}
	return 22050;
}

unsigned char CVersion5Stream::GetChannels()
{
	return m_Stereo ? 2 : 1;
}
