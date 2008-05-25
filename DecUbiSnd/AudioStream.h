// AudioStream.h : Audio stream decoding abstract class
//

#pragma once
#ifndef min
#define min(a, b) ((a)<(b) ? (a) : (b))
#endif

// Provides audio stream decoding
class CAudioStream
{
protected:
	struct SParamDef;
	typedef bool (CAudioStream::*TSetLongParamProc)(unsigned long Value);
	typedef bool (CAudioStream::*TSetStringParamProc)(std::string Value);
	typedef unsigned long (CAudioStream::*TGetLongParamProc)() const;
	typedef std::string (CAudioStream::*TGetStringParamProc)() const;

protected:
	std::istream& m_Input;
	std::streamoff m_BeginOffset;
	std::streamoff m_EndOffset;
	std::map<std::string, SParamDef>* m_Params;

protected:
	virtual void RegisterParam(std::string Name, TSetLongParamProc SetLong, TSetStringParamProc SetString, TGetLongParamProc GetLong, TGetStringParamProc GetString);

public:
	CAudioStream(std::istream& Input, std::streamsize Size);
	CAudioStream(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~CAudioStream();

	virtual unsigned long RecommendBufferLength() const;
	virtual bool InitializeHeader()=0;
	virtual bool Decode(short* Buffer, unsigned long& NumberSamples, unsigned long MaxInputBytes=0xFFFFFFFF)=0;
	virtual bool DecodeToFile(std::ostream& Output, unsigned long& NumberSamples);
	virtual unsigned long GetSampleRate() const=0;
	virtual unsigned char GetChannels() const=0;
	virtual std::string GetFormatName() const=0;

	virtual bool SetParam(std::string Name, long Value);
	virtual bool SetParam(std::string Name, std::string Value);
	virtual long GetLongParam(std::string Name) const;
	virtual std::string GetStringParam(std::string Name) const;
	virtual void GetParamList(std::vector<std::string> Params) const;
};
