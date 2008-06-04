// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Provides UbiSoft version 8 interleaved audio stream decoding
class CInterleavedStream : public CStreamHelper
{
public:
	enum EAudioType
	{
		AT_PCM,
		AT_ADPCM,
		AT_OGGVORBIS
	};

protected:
	struct SInterleavedLayer;

protected:
	EAudioType m_Type;
	unsigned long m_Layer;
	unsigned long m_NumberBlocks;
	std::vector<SInterleavedLayer*> m_Layers;
	unsigned long m_SampleRate;
	unsigned char m_Channels;

protected:
	virtual bool DoDecodeBlock();
	virtual bool DoReadBlock();
	void DoRegisterParams();
	void ClearLayers();

public:
	CInterleavedStream(CDataStream* Input);
	virtual ~CInterleavedStream();

	virtual bool SetLayer(long Layer);
	virtual long GetLayer() const;
	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, unsigned char Force=0);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual EAudioType GetType() const;
};
