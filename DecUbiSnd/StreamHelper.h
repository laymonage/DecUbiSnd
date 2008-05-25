// StreamHelper.h : Contains some helper functions for streams
//

#pragma once
#include "AudioStream.h"

// Helper class for decoding streams
class CStreamHelper : public CAudioStream
{
protected:
	unsigned char* m_InputBuffer;
	short* m_OutputBuffer;
	unsigned long m_InputBufferLength;
	unsigned long m_OutputBufferLength;
	unsigned long m_InputBufferOffset;
	unsigned long m_OutputBufferOffset;
	unsigned long m_InputBufferUsed;
	unsigned long m_OutputBufferUsed;

protected:
	virtual bool DoDecodeBlock(unsigned long MaxInputBytes)=0;
	virtual void PrepareInputBuffer(unsigned long InputBufferLength);
	virtual void PrepareOutputBuffer(unsigned long OutputBufferLength);
	virtual bool FillInputBuffer(unsigned long NumberBytes);
	virtual unsigned long GetInputBytesLeft(unsigned long MaxNumberBytes=0xFFFFFFFF);

public:
	CStreamHelper(std::istream& Input, std::streamsize Size);
	CStreamHelper(std::istream& Input, std::streamoff Offset, std::streamsize Size);
	virtual ~CStreamHelper();

	virtual bool Decode(short* Buffer, unsigned long& NumberSamples, unsigned long MaxInputBytes=0xFFFFFFFF);
};
