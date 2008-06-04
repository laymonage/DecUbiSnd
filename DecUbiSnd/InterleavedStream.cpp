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
	CStreamHelper(Input),
	m_Type(AT_PCM),
	m_Layer(0),
	m_NumberBlocks(0),
	m_SampleRate(48000),
	m_Channels(2)
{
	DoRegisterParams();
	return;
}

CInterleavedStream::~CInterleavedStream()
{
	ClearLayers();
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
		throw(XUserException("The number of channels must 1 or 2"));
	}
	if(!(Force==0 || Force==8))
	{
		throw(XUserException("Cannot force a file to be invalid (must be 0 or 8)"));
	}

	// Clear previous data
	ClearLayers();

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
	m_InputStream->ExactRead(&m_NumberBlocks, 4);
	m_InputStream->ExactIgnore(4);
	m_InputStream->ExactIgnore(4);
	m_InputStream->ExactIgnore(4);

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
		else if(memcmp(Buffer, "OggS", 4)==0)
		{
			// It is Ogg Vorbis
			COggVorbisStream* Stream=new COggVorbisStream(Layer.Data);
			Layer.Type=AT_OGGVORBIS;
			Layer.Stream=Stream;
		}
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
						ClearLayers();
						return false;
					}

					m_SampleRate=Layer.Stream->GetSampleRate();
					m_Channels=Layer.Stream->GetChannels();
				}
				catch(XNeedBuffer&)
				{
					// Get some data
					if(!DoReadBlock())
					{
						break;
					}
					continue;
				}
				break;
			}
		}
	}
	return true;
}

bool CInterleavedStream::DoDecodeBlock()
{
	// Check the state
	if(m_Layer<0 || m_Layer>=m_Layers.size())
	{
		throw(XUserException("The layer number is not valid"));
	}

	// Check the number of blocks
	if(m_NumberBlocks==0)
	{
		return true;
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
		SInterleavedLayer& Layer=*m_Layers[i];

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
		//Layer.Data->EndStream();

		// Grab as much data as we need
		while(true)
		{
			try
			{
				if(Layer.Stream)
				{
					m_OutputBufferUsed=RequestAmount;
					if(!Layer.Stream->Decode(m_OutputBuffer, m_OutputBufferUsed))
					{
						return false;
					}
				}
				else
				{
					// Assume it's aligned
					RequestAmount=Layer.Data->GetBufferedLength();
					m_OutputBufferUsed=Layer.Data->Read(m_OutputBuffer, RequestAmount)/2;
				}
			}
			catch(XNeedBuffer&)
			{
				// Get some data
				if(!DoReadBlock())
				{
					break;
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
	return "ubiinterl8";
}

CInterleavedStream::EAudioType CInterleavedStream::GetType() const
{
	return m_Type;
}

void CInterleavedStream::DoRegisterParams()
{
	RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	return;
}

void CInterleavedStream::ClearLayers()
{
	for(std::vector<SInterleavedLayer*>::iterator Iter=m_Layers.begin();Iter!=m_Layers.end();++Iter)
	{
		delete *Iter;
	}
	m_Layers.clear();
	return;
}
