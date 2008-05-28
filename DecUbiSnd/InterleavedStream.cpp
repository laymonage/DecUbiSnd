// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#include "stdafx.h"
#include "InterleavedStream.h"
#include "Adpcm.h"

CInterleavedStream::CInterleavedStream(std::istream& Input, std::streamsize Size) :
	CStreamHelper(Input, Size),
	m_Type(8),
	m_SubType(ST_UNKNOWN),
	m_Layer(0),
	m_NumberBlocks(0)
{
	DoRegisterParams();
	return;
}

CInterleavedStream::CInterleavedStream(std::istream& Input, std::streamoff Offset, std::streamsize Size) :
	CStreamHelper(Input, Offset, Size),
	m_Type(8),
	m_SubType(ST_UNKNOWN),
	m_Layer(0),
	m_NumberBlocks(0)
{
	DoRegisterParams();
	return;
}

CInterleavedStream::~CInterleavedStream()
{
	return;
}

bool CInterleavedStream::SetLayer(long Layer)
{
	if(Layer<0)
	{
		return false;
	}
	m_Layer=Layer;
	return true;
}

long CInterleavedStream::GetLayer() const
{
	return m_Layer;
}

bool CInterleavedStream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool CInterleavedStream::InitializeHeader(unsigned char Channels, unsigned char Force)
{
	// Check the parameters
	if(Channels<0 || Channels>2)
	{
		return false;
	}
	if(!(Force==0 || Force==8))
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
	m_Input.read((char*)&m_Type, 2);
	if(Force)
	{
		m_Type=Force;
	}
	else
	{
		if(m_Type!=8)
		{
			return false;
		}
	}

	// Read the first header
	unsigned long NumberLayers;
	unsigned short SubType;
	m_Input.read((char*)&SubType, 2);
	m_Input.seekg(4, std::ios_base::cur);
	m_Input.read((char*)&NumberLayers, 4);
	m_Input.read((char*)&m_NumberBlocks, 4);
	m_Input.seekg(4, std::ios_base::cur);
	m_Input.seekg(4, std::ios_base::cur);
	m_Input.seekg(4, std::ios_base::cur);
	m_SubType=(ESubType)SubType;

	// Warn if the sub-type is not recognized
	if(m_SubType!=ST_PCM && m_SubType!=ST_ADPCM)
	{
		std::cerr << "Warning: Unknown sub-type " << m_SubType << "." << std::endl;
	}

	// Process the second header
	std::vector<unsigned long> HeaderSizes;
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		SInterleavedLayer Layer;

		// Read the audio header size
		unsigned long HeaderSize;
		m_Input.read((char*)&HeaderSize, 4);
		HeaderSizes.push_back(HeaderSize);

		// Add it
		Layer.First=true;
		m_Layers.push_back(Layer);
	}

	// Read the audio headers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Some variables
		const unsigned long HeaderSize=HeaderSizes[i];
		const std::streamoff SeekToOffset=(std::streamoff)m_Input.tellg()+HeaderSize;

		// Process each header based on what type this is
		if(m_SubType==ST_ADPCM)
		{
			// Check the header size
			if(HeaderSize<28)
			{
				std::cerr << "Error: Header size is unrecognized and too small (ADPCM, " << HeaderSize << " bytes, should be " << 28 << ")" << std::endl;
				return false;
			}
			else if(HeaderSize!=28)
			{
				std::cerr << "Warning: Header size is unrecognized (ADPCM, " << HeaderSize << " bytes, should be " << 28 << ")" << std::endl;
			}

			// Read the header
			if(m_Input.get()!=5)
			{
				std::cerr << "Warning: This doesn't appear to be the standard header for interleaved streams" << std::endl;
			}
			m_Input.seekg(13, std::ios_base::cur);
			m_Input.read((char*)&m_Layers[i].NumberExtraSamples, 2);
			m_Input.read((char*)&m_Layers[i].LeftSample, 2);
			m_Input.read((char*)&m_Layers[i].LeftIndex, 1);
			m_Input.seekg(1, std::ios_base::cur);
			m_Input.read((char*)&m_Layers[i].RightSample, 2);
			m_Input.read((char*)&m_Layers[i].RightIndex, 1);
			m_Input.seekg(5, std::ios_base::cur);

			// Check if it's stereo
			if(Channels==0)
			{
				// TODO: Scan through the data mono, then stereo and see which one makes sense
				std::cerr << "Warning: Automatic channels detection has not yet been implemented. Assuming stereo." << std::endl;
				m_Layers[i].Stereo=true;
			}
			else if(Channels==1)
			{
				m_Layers[i].Stereo=false;
			}
			else if(Channels==2)
			{
				m_Layers[i].Stereo=true;
			}

			// Give a warning if the number of extra samples is unrecognized
			if(m_Layers[i].NumberExtraSamples!=10)
			{
				std::cerr << "Warning: The number of extra uncompressed samples is unrecognized (" << m_Layers[i].NumberExtraSamples << " samples)" << std::endl;
			}
			if(m_Layers[i].Stereo)
			{
				m_Layers[i].NumberExtraSamples*=2;
			}
		}
		else if(m_SubType==ST_PCM)
		{
			// This should have no header
			if(HeaderSize!=0)
			{
				std::cerr << "Warning: Header size is unrecognized (PCM, " << HeaderSize << " bytes, should be " << 0 << ")" << std::endl;
			}
		}

		// Seek to the position we should be at
		m_Input.seekg(SeekToOffset);
	}
	return true;
}

bool CInterleavedStream::DoDecodeBlock(unsigned long MaxInputBytes)
{
	// Check the state
	if(m_Layer<0 || m_Layer>=m_Layers.size())
	{
		return false;
	}

	// Check the number of blocks
	if(m_NumberBlocks==0)
	{
		return true;
	}

	// Is there any left?
	if(!GetInputBytesLeft(MaxInputBytes))
	{
		return true;
	}

	// Read and process the subheader
	unsigned long LayerSum=0;
	m_Input.seekg(8, std::ios_base::cur);
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Read the block size
		m_Input.read((char*)&m_Layers[i].BlockSize, 4);
		LayerSum+=m_Layers[i].BlockSize;
	}

	// Is there any left?
	if(!GetInputBytesLeft(MaxInputBytes))
	{
		return true;
	}

	// Prepare the buffers
	PrepareInputBuffer(LayerSum);
	PrepareOutputBuffer(LayerSum*2);
	FillInputBuffer(min(m_InputBufferLength,  MaxInputBytes));

	// Is there any input?
	if(m_InputBufferUsed<LayerSum)
	{
		return true;
	}

	// Read from each of the streams
	m_OutputBufferUsed=0;
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		unsigned long AudioSize=m_Layers[i].BlockSize;
		unsigned long OutputSize=0;

		// Subtract the first header from it
		if(m_Layers[i].First)
		{
			if(m_SubType==ST_ADPCM && m_Layers[i].NumberExtraSamples)
			{
				// Copy the data
				memcpy(m_OutputBuffer+m_OutputBufferUsed, m_InputBuffer+m_InputBufferOffset, m_Layers[i].NumberExtraSamples*2);

				// Update the positions
				m_InputBufferOffset+=m_Layers[i].NumberExtraSamples*2;
				m_OutputBufferUsed+=m_Layers[i].NumberExtraSamples;
				AudioSize-=m_Layers[i].NumberExtraSamples*2;
				m_Layers[i].NumberExtraSamples=0;
			}
			m_Layers[i].First=false;
		}

		// Decode the audio
		if(m_SubType==ST_ADPCM)
		{
			if(!m_Layers[i].Stereo)
			{
				SAdpcmMonoParam Param;
				Param.InputBuffer=m_InputBuffer+m_InputBufferOffset;
				Param.InputLength=AudioSize;
				Param.OutputBuffer=m_OutputBuffer+m_OutputBufferUsed;
				Param.FirstSample=m_Layers[i].LeftSample;
				Param.FirstIndex=m_Layers[i].LeftIndex;
				DecompressMonoAdpcm(&Param);
				m_Layers[i].LeftSample=Param.FirstSample;
				m_Layers[i].LeftIndex=Param.FirstIndex;
				m_Layers[i].RightSample=0;
				m_Layers[i].RightIndex=0;
			}
			else
			{
				SAdpcmStereoParam Param;
				Param.InputBuffer=m_InputBuffer+m_InputBufferOffset;
				Param.InputLength=AudioSize;
				Param.OutputBuffer=m_OutputBuffer+m_OutputBufferUsed;
				Param.FirstLeftSample=m_Layers[i].LeftSample;
				Param.FirstLeftIndex=m_Layers[i].LeftIndex;
				Param.FirstRightSample=m_Layers[i].RightSample;
				Param.FirstRightIndex=m_Layers[i].RightIndex;
				DecompressStereoAdpcm(&Param);
				m_Layers[i].LeftSample=Param.FirstLeftSample;
				m_Layers[i].LeftIndex=Param.FirstLeftIndex;
				m_Layers[i].RightSample=Param.FirstRightSample;
				m_Layers[i].RightIndex=Param.FirstRightIndex;
			}
			OutputSize=AudioSize*2;
		}
		else
		{
			memcpy(m_OutputBuffer+m_OutputBufferUsed, m_InputBuffer+m_InputBufferOffset, AudioSize);
			OutputSize=AudioSize/2;
		}

		// Update the positions
		m_InputBufferOffset+=AudioSize;

		// Only advance the counter if this is the layer we want to keep, 
		// otherwise it will be overwritten
		if(i==m_Layer)
		{
			m_OutputBufferUsed+=OutputSize;
		}
	}

	// Update some variables
	m_InputBufferOffset=m_InputBufferUsed;
	m_NumberBlocks--;
	return true;
}

unsigned long CInterleavedStream::GetSampleRate() const
{
	// Check each possible type
	if(m_Type==8)
	{
		return 48000;
	}
	return 22050;
}

unsigned char CInterleavedStream::GetChannels() const
{
	return 2;
}

std::string CInterleavedStream::GetFormatName() const
{
	// Check each possible type
	if(m_Type==8)
	{
		return "ubiinterl8";
	}
	return "unknown";
}

CInterleavedStream::ESubType CInterleavedStream::GetSubType() const
{
	return m_SubType;
}

void CInterleavedStream::DoRegisterParams()
{
	RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	return;
}
