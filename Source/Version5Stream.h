// Version5Stream.h : UbiSoft version 3 and 5 audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Provides UbiSoft version 3 and 5 audio stream decoding
class CVersion5Stream : public CStreamHelper
{
protected:
	unsigned char m_Type;
	unsigned short m_NumberExtraSamples;
	bool m_Stereo;
	short m_LeftSample;
	short m_RightSample;
	unsigned char m_LeftIndex;
	unsigned char m_RightIndex;

protected:
	virtual bool DoDecodeBlock();

public:
	CVersion5Stream(CDataStream* Input);
	virtual ~CVersion5Stream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, unsigned char Force=0);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
