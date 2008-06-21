// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#include "stdafx.h"
#include "InterleavedStream.h"
#include "Version5Stream.h"
#include "OggVorbisStream.h"
#include "BufferDataStream.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"

// Information associated with each layer
struct CInterleavedStream::SInterleavedLayer
{
	SInterleavedLayer() : Stream(NULL), Data(NULL) {}
	~SInterleavedLayer() { delete Stream; delete Data; }

	EAudioType Type;
	CAudioStream* Stream;
	CBufferDataStream* Data;
	bool First;
	unsigned long BlockSize;
};

CInterleavedStream::CInterleavedStream(CDataStream* Input) :
	CLayeredStreamHelper(Input),
	m_NumberBlocks(0),
	m_SampleRate(48000),
	m_Channels(2),
	m_TotalBlocks(0)
{
	DoRegisterParams();
	return;
}

CInterleavedStream::~CInterleavedStream()
{
	Clear();
	return;
}

bool CInterleavedStream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool CInterleavedStream::InitializeHeader(unsigned char Channels, unsigned char Force)
{
	// Check the parameters
	if(Channels<1 || Channels>2)
	{
		throw(XUserException("The number of channels must 1 or 2"));
	}
	if(!(Force==0 || Force==8))
	{
		throw(XUserException("Cannot force a file to be invalid (must be 0 or 8)"));
	}

	// Clear previous data
	Clear();

	// Set the stereo flag
	if(Channels==1)
	{
		m_Channels=1;
	}
	else if(Channels==2 || Channels==0)
	{
		m_Channels=2;
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
		if(Type!=8)
		{
			throw(XFileException("File does not have the correct signature (should be 08)"));
		}
	}

	// Read the first header
	unsigned long NumberLayers;
	unsigned short SubType;
	m_InputStream->ExactRead(&SubType, 2);
	m_InputStream->ExactIgnore(4);
	m_InputStream->ExactRead(&NumberLayers, 4);
	m_InputStream->ExactRead(&m_TotalBlocks, 4);
	m_InputStream->ExactIgnore(4);
	m_InputStream->ExactIgnore(4);
	m_InputStream->ExactIgnore(4);
	m_NumberBlocks=m_TotalBlocks;

	// Process the second header
	std::vector<unsigned long> HeaderSizes;
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Read the audio header size
		unsigned long HeaderSize;
		m_InputStream->ExactRead(&HeaderSize, 4);
		HeaderSizes.push_back(HeaderSize);
	}

	// Read the headers and create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Create the layer
		SInterleavedLayer* NewLayer=new SInterleavedLayer;
		m_Layers.push_back(NewLayer);
		SInterleavedLayer& Layer=*m_Layers[i];

		// Read the header and send it
		unsigned char* Buffer;
		Layer.Data=new CBufferDataStream();
		Buffer=(unsigned char*)m_InputStream->ExactRead(HeaderSizes[i]);
		if(HeaderSizes[i]>0)
		{
			Layer.Data->SendBuffer(Buffer, HeaderSizes[i]);
		}

		// Detect the type
		if(HeaderSizes[i]==0)
		{
			// Must be PCM, there is no header
			Layer.Type=AT_PCM;
			Layer.Stream=NULL;
		}
		else if((Buffer[0]==5 || Buffer[0]==3) && HeaderSizes[i]>=28)
		{
			// Check the header size
			if(HeaderSizes[i]!=28)
			{
				std::cerr << "Warning: Header size is unrecognized (ADPCM, " << HeaderSizes[i] << " bytes, should be " << 28 << ")" << std::endl;
			}

			// It is likely a simple block
			CVersion5Stream* Stream=new CVersion5Stream(Layer.Data);
			Layer.Type=AT_ADPCM;
			Layer.Stream=Stream;

			// Initialize the header
			try
			{
				// Initialize the header
				if(!Stream->InitializeHeader(Channels))
				{
					Clear();
					return false;
				}
			}
			catch(XNeedBuffer&)
			{
				Clear();
				throw(XFileException("The decoder needed more information than the header provided"));
			}
		}
		else if(memcmp(Buffer, "OggS", 4)==0)
		{
			// It is Ogg Vorbis
			COggVorbisStream* Stream=new COggVorbisStream(Layer.Data);
			Layer.Type=AT_OGGVORBIS;
			Layer.Stream=Stream;
		}
	}

	// Set the callback functions now that we're able to start reading blocks
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// Set it
		Layer.Data->SetNeedBufferCallback(BufferCallback, this);
	}

	// Initialize the headers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// Initialize the headers
		if(Layer.Type==AT_PCM)
		{
			// Raw PCM has no header
		}
		else if(Layer.Type==AT_ADPCM)
		{
			// Already initialized
		}
		else if(Layer.Type==AT_OGGVORBIS)
		{
			// Grab as much data as we need
			while(true)
			{
				try
				{
					// Initialize the header
					if(!Layer.Stream->InitializeHeader())
					{
						Clear();
						return false;
					}

					m_SampleRate=Layer.Stream->GetSampleRate();
					m_Channels=Layer.Stream->GetChannels();
				}
				catch(XNeedBuffer& e)
				{
					while(Layer.Data->GetBufferedLength()<e.GetLength())
					{
						// Get some data
						if(!DoReadBlock())
						{
							break;
						}
					}
					continue;
				}
				break;
			}
		}
	}

	// We're initialized
	m_Initialized=true;
	return true;
}

bool CInterleavedStream::DoDecodeLayer(unsigned long LayerIndex)
{
	// Make sure it's valid
	if(!m_Initialized)
	{
		throw(XProgramException("The stream has not been initialized"));
	}

	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// If this is not the layer just continue
		if(i!=LayerIndex)
		{
			if(!IsLayerDecoded(i))
			{
				Layer.Data->ResetBuffer();
			}
			continue;
		}

		// Prepare the output
		unsigned long RequestAmount=RecommendBufferLength();
		PrepareOutputBuffer(RequestAmount);

		// Grab as much data as we need
		while(true)
		{
			try
			{
				if(Layer.Stream)
				{
					if(!Layer.Stream->Decode(m_OutputBuffer, RequestAmount))
					{
						return false;
					}
					//std::cout << RequestAmount << std::endl;
					m_OutputBufferUsed=RequestAmount;
				}
				else
				{
					m_OutputBufferUsed=Layer.Data->Read(m_OutputBuffer, RequestAmount)/2;
				}
			}
			catch(XNeedBuffer& e)
			{
				while(Layer.Data->GetBufferedLength()<e.GetLength())
				{
					// Get some data
					if(!DoReadBlock())
					{
						break;
					}
				}
				continue;
			}
			break;
		}
	}
	return true;
}

bool CInterleavedStream::DoReadBlock()
{
	// Check for the end of the file
	if(m_InputStream->IsEnd() || m_NumberBlocks==0)
	{
		// Mark the end of the stream for all of the layers
		for(unsigned long i=0;i<m_Layers.size();i++)
		{
			SInterleavedLayer& Layer=*m_Layers[i];
			Layer.Data->EndStream();
		}
		return false;
	}

	// Process the first block header
	unsigned long BlockID;
	m_InputStream->ExactRead(&BlockID, 4);
	if(BlockID!=3)
	{
		throw(XFileException("Error: Invalid block ID"));
	}
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
		SInterleavedLayer& Layer=*m_Layers[i];
		Layer.First=false;

		// Feed it a block
		void* Buffer;
		Buffer=m_InputStream->ExactRead(BlockSizes[i]);
		Layer.Data->SendBuffer(Buffer, BlockSizes[i]);
	}
	m_NumberBlocks--;
	return true;
}

unsigned long CInterleavedStream::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char CInterleavedStream::GetChannels() const
{
	return m_Channels;
}

std::string CInterleavedStream::GetFormatName() const
{
	return "ubi_iv8";
}

CInterleavedStream::EAudioType CInterleavedStream::GetType(unsigned long Layer) const
{
	// Check the state
	if(Layer<0 || Layer>=m_Layers.size())
	{
		throw(XUserException("The layer number is not valid"));
	}

	// Get a reference to the layer
	const SInterleavedLayer& LayerRef=*m_Layers[Layer];
	return LayerRef.Type;
}

unsigned long CInterleavedStream::GetNumberBlocks() const
{
	return m_TotalBlocks;
}

unsigned long CInterleavedStream::GetLayerCount() const
{
	return (unsigned long)m_Layers.size();
}

void CInterleavedStream::DoRegisterParams()
{
	//RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	return;
}

void CInterleavedStream::Clear()
{
	for(std::vector<SInterleavedLayer*>::iterator Iter=m_Layers.begin();Iter!=m_Layers.end();++Iter)
	{
		delete *Iter;
	}
	m_Layers.clear();
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;
	m_Initialized=false;
	return;
}

void CInterleavedStream::BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData)
{
	// Check the user data
	if(!UserData)
	{
		return;
	}

	// Get the audio stream class
	CInterleavedStream& Audio=*(CInterleavedStream*)UserData;

	// Read in as much data as needed
	while(Stream.GetBufferedLength()<Bytes)
	{
		// Get some data
		if(!Audio.DoReadBlock())
		{
			break;
		}
	}
	return;
}
