// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Information associated with each stream
struct SInterleavedLayer
{
	bool Stereo;
	short LeftSample;
	short RightSample;
	unsigned char LeftIndex;
	unsigned char RightIndex;
	unsigned long BlockSize;
	bool First;
};

// Provides UbiSoft version 3 and 5 audio stream decoding
class CInterleavedStream : public CStreamHelper
{
public:
	enum ESubType
	{
		ST_UNKNOWN=0,
		ST_ADPCM=11,
		ST_PCM=16
	};

protected:
	unsigned short m_Type;
	ESubType m_SubType;
	unsigned long m_Layer;
	unsigned long m_NumberBlocks;
	std::vector<SInterleavedLayer> m_Layers;

protected:
	virtual bool DoDecodeBlock(unsigned long MaxInputBytes);
	void DoRegisterParams();

public:
	CInterleavedStream(std::istream& Input, std::streamsize Size);
	CInterleavedStream(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~CInterleavedStream();

	virtual bool SetLayer(long Layer);
	virtual long GetLayer();
	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, unsigned char Force=0);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual ESubType GetSubType() const;
};