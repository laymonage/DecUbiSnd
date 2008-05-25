// Version5Stream.cpp : UbiSoft version 3 and 5 audio stream decoding
//

#include "stdafx.h"
#include "Version5Stream.h"
#include "Adpcm.h"

CVersion5Stream::CVersion5Stream(std::istream& Input, std::streamsize Size) :
	CStreamHelper(Input, Size),
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
	CStreamHelper(Input, Offset, Size),
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

	// The samples are big-endian for some reason
	if(m_Type==3)
	{
		m_LeftSample=(((unsigned short)m_LeftSample&0xFF00)>>8) | (((unsigned short)m_LeftSample&0x00FF)<<8);
		m_RightSample=(((unsigned short)m_RightSample&0xFF00)>>8) | (((unsigned short)m_RightSample&0x00FF)<<8);
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

bool CVersion5Stream::DoDecodeBlock(unsigned long MaxInputBytes)
{
	// Prepare the buffers
	PrepareInputBuffer(RecommendBufferLength()/2);
	PrepareOutputBuffer(RecommendBufferLength());
	FillInputBuffer(min(m_InputBufferLength,  MaxInputBytes));

	// Calculate how many samples are needed
	m_OutputBufferUsed=m_InputBufferUsed*2;
	if(m_OutputBufferUsed<1)
	{
		return true;
	}

	// Do the decompression
	if(!m_Stereo)
	{
		SAdpcmMonoParam Param;
		Param.InputBuffer=m_InputBuffer;
		Param.InputLength=m_InputBufferUsed;
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
		Param.InputBuffer=m_InputBuffer;
		Param.InputLength=m_InputBufferUsed;
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

	// All of the input buffer was used
	m_InputBufferOffset=m_InputBufferUsed;
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
