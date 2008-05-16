// AudioStream.h : Audio stream decoding abstract class
//

// Provides audio stream decoding
class CAudioStream
{
protected:
	std::istream& m_Input;
	std::streamoff m_BeginOffset;
	std::streamoff m_EndOffset;

public:
	CAudioStream(std::istream& Input, std::streamsize Size);
	CAudioStream(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~CAudioStream();

	virtual unsigned long RecommendBufferLength();
	virtual bool Decode(short* Buffer, unsigned long& NumberSamples)=0;
	virtual unsigned long GetSampleRate()=0;
	virtual unsigned char GetChannels()=0;
};
