// OldInterleavedStream.cpp : UbiSoft version 2 interleaved audio stream decoding
//

#include "stdafx.h"
#include "OldInterleavedStream.h"
#include "Version5Stream.h"

struct COldInterleavedStream::SOldInterleavedLayer
{
	SOldInterleavedLayer() : Stream(NULL) {};
	CVersion5Stream* Stream;
	std::streamoff NextOffset;

	unsigned long BlockSize;
	bool First;
};

COldInterleavedStream::COldInterleavedStream(std::istream& Input, std::streamsize Size) :
	CStreamHelper(Input, Size),
	m_Layer(0),
	m_BlockNumber(0)
{
	DoRegisterParams();
	m_Layers=new std::vector<SOldInterleavedLayer>;
	return;
}

COldInterleavedStream::COldInterleavedStream(std::istream& Input, std::streamoff Offset, std::streamsize Size) :
	CStreamHelper(Input, Offset, Size),
	m_Layer(0),
	m_BlockNumber(0)
{
	DoRegisterParams();
	m_Layers=new std::vector<SOldInterleavedLayer>;
	return;
}

COldInterleavedStream::~COldInterleavedStream()
{
	ClearLayers();
	delete m_Layers;
	return;
}

bool COldInterleavedStream::SetLayer(long Layer)
{
	if(Layer<0)
	{
		return false;
	}
	m_Layer=Layer;
	return true;
}

long COldInterleavedStream::GetLayer() const
{
	return m_Layer;
}

bool COldInterleavedStream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool COldInterleavedStream::InitializeHeader(unsigned char Channels, unsigned char Force)
{
	// Check the parameters
	if(Channels<0 || Channels>2)
	{
		return false;
	}
	if(!(Force==0 || Force==2))
	{
		return false;
	}

	// Check the input
	if((m_EndOffset-m_BeginOffset)<100)
	{
		return false;
	}

	// Clear previous data
	ClearLayers();

	// Read the type from the file
	m_Input.seekg(m_BeginOffset);
	m_Input.read((char*)&m_Type, 2);
	if(Force)
	{
		m_Type=Force;
	}
	else
	{
		if(m_Type!=2)
		{
			return false;
		}
	}

	// Read the header
	unsigned long NumberLayers;
	unsigned long TotalSize;
	m_Input.seekg(2, std::ios_base::cur);
	m_Input.read((char*)&NumberLayers, 4);
	m_Input.read((char*)&TotalSize, 4);
	m_Input.seekg(12, std::ios_base::cur);

	// Change the end of the stream, if necessary
	if((std::streamoff)TotalSize<m_EndOffset && (std::streamoff)TotalSize+m_BeginOffset<m_EndOffset)
	{
		m_EndOffset=TotalSize+m_BeginOffset;
	}

	// A check
	if(NumberLayers!=3)
	{
		std::cerr << "Information: " << NumberLayers << " layers" << std::endl;
	}

	// Process the first block header
	std::streamoff FirstBlock=m_Input.tellg();
	std::streamoff NextData=FirstBlock+8+NumberLayers*4;
	unsigned long BlockID;
	m_Input.read((char*)&BlockID, 4);
	if(BlockID!=1)
	{
		std::cerr << "Error: Invalid block ID" << std::endl;
		return false;
	}
	m_BlockNumber=1;
	m_Input.seekg(4, std::ios_base::cur);

	// Create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Read the block size
		unsigned long BlockSize;
		m_Input.read((char*)&BlockSize, 4);

		// Add a new layer
		SOldInterleavedLayer NewLayer;
		m_Layers->push_back(NewLayer);
		SOldInterleavedLayer& Layer=(*m_Layers)[i];

		// Create a new stream for the layer
		std::streamoff PrevOffset=m_Input.tellg();
		Layer.Stream=new CVersion5Stream(m_Input, NextData, m_EndOffset-NextData);
		Layer.First=true;
		NextData+=BlockSize;

		// Initialize the header
		if(!Layer.Stream->InitializeHeader(Channels))
		{
			ClearLayers();
			return false;
		}
		m_Input.seekg(PrevOffset);
	}
	m_Input.seekg(FirstBlock);
	return true;
}

bool COldInterleavedStream::DoDecodeBlock(unsigned long MaxInputBytes)
{
	//__asm int 3;
	// Check the state
	if(m_Layer<0 || m_Layer>=m_Layers->size())
	{
		return false;
	}

	// Is there any left?
	if(!GetInputBytesLeft(MaxInputBytes))
	{
		return true;
	}

	// Process the block header
	unsigned long BlockID;
	m_Input.read((char*)&BlockID, 4);
	if(BlockID!=m_BlockNumber)
	{
		std::cerr << "Error: Invalid block ID" << std::endl;
		return false;
	}
	m_BlockNumber++;
	m_Input.seekg(4, std::ios_base::cur);

	// Read the layer sizes, only keeping what we need
	unsigned long BlockBeginSkip=0;
	unsigned long BlockEndSkip=0;
	unsigned long CurrentBlockSize=0;
	for(unsigned long i=0;i<m_Layers->size();i++)
	{
		unsigned long BlockSize;
		m_Input.read((char*)&BlockSize, 4);
		if(i<m_Layer)
		{
			BlockBeginSkip+=BlockSize;
		}
		else if(i>m_Layer)
		{
			BlockEndSkip+=BlockSize;
		}
		else if(i==m_Layer)
		{
			CurrentBlockSize=BlockSize;
		}

		if(m_Layer!=i && (*m_Layers)[i].First)
		{
			(*m_Layers)[i].First=false;
		}
	}

	// Do the skip
	m_Input.seekg(BlockBeginSkip, std::ios_base::cur);

	// Set things up
	SOldInterleavedLayer& Layer=(*m_Layers)[m_Layer];
	if(Layer.First)
	{
		m_Input.seekg(28, std::ios_base::cur);
		CurrentBlockSize-=28;
		Layer.First=false;
	}

	// Decode a block
	PrepareOutputBuffer(CurrentBlockSize*2);
	m_OutputBufferUsed=CurrentBlockSize*2;
	if(!Layer.Stream->Decode(m_OutputBuffer, m_OutputBufferUsed, CurrentBlockSize))
	{
		return false;
	}

	// Do the skip
	m_Input.seekg(BlockEndSkip, std::ios_base::cur);
	return true;
}

unsigned long COldInterleavedStream::GetSampleRate() const
{
	// Check each possible type
	if(m_Type==2)
	{
		return 36000;
	}
	return 22050;
}

unsigned char COldInterleavedStream::GetChannels() const
{
	return 2;
}

std::string COldInterleavedStream::GetFormatName() const
{
	// Check each possible type
	if(m_Type==2)
	{
		return "ubiinterl2";
	}
	return "unknown";
}

void COldInterleavedStream::DoRegisterParams()
{
	RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	return;
}

void COldInterleavedStream::ClearLayers()
{
	for(std::vector<SOldInterleavedLayer>::iterator Iter=m_Layers->begin();Iter!=m_Layers->end();++Iter)
	{
		delete Iter->Stream;
		Iter->Stream=NULL;
	}
	m_Layers->clear();
	return;
}
