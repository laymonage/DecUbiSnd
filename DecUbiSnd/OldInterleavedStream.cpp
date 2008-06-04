// OldInterleavedStream.cpp : UbiSoft version 2 interleaved audio stream decoding
//

#include "stdafx.h"
#include "OldInterleavedStream.h"
#include "Version5Stream.h"
#include "BufferDataStream.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"

// Information associated with each layer
struct COldInterleavedStream::SOldInterleavedLayer
{
	SOldInterleavedLayer() : Stream(NULL), Data(NULL) {};
	~SOldInterleavedLayer() { delete Stream; delete Data; }

	CVersion5Stream* Stream;
	CBufferDataStream* Data;
	bool First;
};

COldInterleavedStream::COldInterleavedStream(CDataStream* Input) :
	CStreamHelper(Input),
	m_Layer(0),
	m_BlockNumber(0),
	m_TotalBytes(0),
	m_SampleRate(36000),
	m_Stereo(true)
{
	DoRegisterParams();
	return;
}

COldInterleavedStream::~COldInterleavedStream()
{
	ClearLayers();
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
	if(Channels<1 || Channels>2)
	{
		throw(XUserException("The number of channels must 1 or 2"));
	}
	if(!(Force==0 || Force==2))
	{
		throw(XUserException("Cannot force a file to be invalid (must be 0 or 2)"));
	}

	// Clear previous data
	ClearLayers();

	// Set the stereo flag
	if(Channels==1)
	{
		m_Stereo=false;
	}
	else if(Channels==2)
	{
		m_Stereo=true;
	}

	// Read the type from the file
	unsigned short Type;
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}
	m_InputStream->ExactRead(&Type, 2);
	if(Force)
	{
		Type=Force;
	}
	else
	{
		if(Type!=2)
		{
			throw(XFileException("File does not have the correct signature (should be 02)"));
		}
	}

	// Read the header
	unsigned long NumberLayers;
	unsigned long TotalSize;
	m_InputStream->ExactIgnore(2);
	m_InputStream->ExactRead(&NumberLayers, 4);
	m_InputStream->ExactRead(&TotalSize, 4);
	m_InputStream->ExactIgnore(12);

	// Set the total number of bytes
	m_TotalBytes=TotalSize;
	m_BlockNumber=1;

	// A check
	if(NumberLayers!=3)
	{
		std::cerr << "Information: " << NumberLayers << " layers" << std::endl;
	}

	// Create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Create the layer
		SOldInterleavedLayer* Layer=new SOldInterleavedLayer;

		// Create a new stream for the layer
		Layer->Data=new CBufferDataStream();
		Layer->Stream=new CVersion5Stream(Layer->Data);
		Layer->First=true;

		// Push it on
		m_Layers.push_back(Layer);
	}

	// Read the first block
	if(!DoReadBlock())
	{
		ClearLayers();
		return false;
	}

	// Initialize the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];

		try
		{
			// Initialize the header
			if(!Layer.Stream->InitializeHeader(m_Stereo ? 2 : 1))
			{
				ClearLayers();
				return false;
			}
		}
		catch(XNeedBuffer&)
		{
			ClearLayers();
			throw(XFileException("The decoder needed more information than the header provided"));
		}
	}
	return true;
}

bool COldInterleavedStream::DoDecodeBlock()
{
	// Check the state
	if(m_Layer<0 || m_Layer>=m_Layers.size())
	{
		throw(XUserException("The layer number is not valid"));
	}

	// Read a block
	if(!DoReadBlock())
	{
		return true;
	}

	// Go through each of the layers, decoding it
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];

		// If this is not the layer just continue
		if(i!=m_Layer)
		{
			Layer.Data->ResetBuffer();
			continue;
		}

		// Prepare the output
		unsigned long RequestAmount=Layer.Data->GetBufferedLength()*2;
		PrepareOutputBuffer(RequestAmount);

		// HACK: I should find a better way of decoding only what is needed
		Layer.Data->EndStream();

		// Decode
		try
		{
			m_OutputBufferUsed=RequestAmount;
			if(!Layer.Stream->Decode(m_OutputBuffer, m_OutputBufferUsed))
			{
				return false;
			}
		}
		catch(XNeedBuffer&)
		{
			return DoDecodeBlock();
		}
	}
	return true;
}

bool COldInterleavedStream::DoReadBlock()
{
	// Check for the end of the file
	if(m_InputStream->IsEnd() || m_InputStream->Tell()>=m_TotalBytes)
	{
		// Mark the end of the stream for all of the layers
		for(unsigned long i=0;i<m_Layers.size();i++)
		{
			SOldInterleavedLayer& Layer=*m_Layers[i];
			Layer.Data->EndStream();
		}
		return false;
	}

	// Process the first block header
	unsigned long BlockID;
	m_InputStream->ExactRead(&BlockID, 4);
	if(BlockID!=m_BlockNumber)
	{
		throw(XFileException("Error: Invalid block ID"));
	}
	m_BlockNumber++;
	m_InputStream->ExactIgnore(4);

	// Read in the block sizes
	std::vector<unsigned long> BlockSizes;
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		unsigned long BlockSize;
		m_InputStream->ExactRead(&BlockSize, 4);
		BlockSizes.push_back(BlockSize);
	}

	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get a reference to the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];
		Layer.First=false;

		// Feed it a block
		void* Buffer;
		Buffer=m_InputStream->ExactRead(BlockSizes[i]);
		Layer.Data->SendBuffer(Buffer, BlockSizes[i]);
	}
	return true;
}

void COldInterleavedStream::SetSampleRate(unsigned long SampleRate)
{
	m_SampleRate=SampleRate;
	return;
}

unsigned long COldInterleavedStream::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char COldInterleavedStream::GetChannels() const
{
	return m_Stereo ? 2 : 1;
}

std::string COldInterleavedStream::GetFormatName() const
{
	return "ubiinterl2";
}

void COldInterleavedStream::DoRegisterParams()
{
	RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	//RegisterParam("SampleRate", (TSetLongParamProc)SetSampleRate, NULL, (TGetLongParamProc)GetSampleRate, NULL);
	return;
}

void COldInterleavedStream::ClearLayers()
{
	for(std::vector<SOldInterleavedLayer*>::iterator Iter=m_Layers.begin();Iter!=m_Layers.end();++Iter)
	{
		delete *Iter;
	}
	m_Layers.clear();
	return;
}
