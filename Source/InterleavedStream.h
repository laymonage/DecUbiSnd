// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#pragma once
#include "LayeredStreamHelper.h"

class CBufferDataStream;

// Provides UbiSoft version 8 interleaved audio stream decoding
class CInterleavedStream : public CLayeredStreamHelper
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
	unsigned long m_NumberBlocks;
	std::vector<SInterleavedLayer*> m_Layers;
	unsigned long m_SampleRate;
	unsigned char m_Channels;
	unsigned long m_TotalBlocks;

protected:
	virtual bool DoDecodeLayer(unsigned long LayerIndex);
	virtual bool DoReadBlock();
	void DoRegisterParams();
	void Clear();

public:
	CInterleavedStream(CDataStream* Input);
	virtual ~CInterleavedStream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned long SampleRate);
	virtual bool InitializeHeader(unsigned long SampleRate, unsigned char PcmChannels);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual EAudioType GetType(unsigned long Layer) const;
	virtual unsigned long GetNumberBlocks() const;
	virtual unsigned long GetLayerCount() const;

private:
	static void BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData);
};
