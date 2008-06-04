// OldInterleavedStream.h : UbiSoft version 2 interleaved audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Provides UbiSoft version 2 interleaved audio stream decoding
class COldInterleavedStream : public CStreamHelper
{
protected:
	struct SOldInterleavedLayer;

protected:
	unsigned long m_Layer;
	unsigned long m_BlockNumber;
	unsigned long m_TotalBytes;
	std::vector<SOldInterleavedLayer*> m_Layers;
	unsigned long m_SampleRate;
	bool m_Stereo;

protected:
	virtual bool DoDecodeBlock();
	virtual bool DoReadBlock();
	void DoRegisterParams();
	void ClearLayers();

public:
	COldInterleavedStream(CDataStream* Input);
	virtual ~COldInterleavedStream();

	virtual bool SetLayer(long Layer);
	virtual long GetLayer() const;
	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, unsigned char Force=0);
	virtual void SetSampleRate(unsigned long SampleRate);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
