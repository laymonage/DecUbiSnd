// Version5Stream.h : UbiSoft version 3 and 5 audio stream decoding
//

#include "AudioStream.h"

// Provides UbiSoft version 3 and 5 audio stream decoding
class CVersion5Stream : public CAudioStream
{
protected:
	unsigned char m_Type;
	bool m_Stereo;
	short m_LeftSample;
	short m_RightSample;
	unsigned char m_LeftIndex;
	unsigned char m_RightIndex;

public:
	CVersion5Stream(std::istream& Input, std::streamsize Size);
	CVersion5Stream(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~CVersion5Stream();

	virtual bool InitHeader(unsigned char Channels, unsigned char Force=0);
	virtual bool Decode(short* Buffer, unsigned long& NumberSamples);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
};
