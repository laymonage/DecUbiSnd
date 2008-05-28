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
	unsigned short m_Type;
	unsigned long m_Layer;
	unsigned long m_BlockNumber;
	std::vector<SOldInterleavedLayer>* m_Layers;

protected:
	virtual bool DoDecodeBlock(unsigned long MaxInputBytes);
	void DoRegisterParams();
	void ClearLayers();

public:
	COldInterleavedStream(std::istream& Input, std::streamsize Size);
	COldInterleavedStream(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~COldInterleavedStream();

	virtual bool SetLayer(long Layer);
	virtual long GetLayer() const;
	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, unsigned char Force=0);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
