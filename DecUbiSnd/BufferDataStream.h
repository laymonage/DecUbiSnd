// BufferDataStream.h : A data stream from buffers
//

#pragma once

#include "DataStream.h"

// A data stream from buffers
class CBufferDataStream : public CDataStream
{
protected:
	unsigned char* m_Buffer;
	unsigned long m_BufferLength;
	unsigned long m_BufferOffset;
	unsigned long m_BufferUsed;
	bool m_StreamEnd;

protected:
	virtual unsigned long DoRead(void* Buffer, unsigned long Length, bool Exact);
	virtual bool DoCanSeekBackward() const;
	virtual unsigned long DoForwardSeek(unsigned long RelOffset);
	virtual bool DoIsEnd() const;
	virtual unsigned long GetUsedBuffer() const;
	virtual unsigned long GetUnusedBuffer() const;

public:
	CBufferDataStream(unsigned long InternalBufferSize=0x20000);
	virtual ~CBufferDataStream();

	virtual void SendBuffer(void* Buffer, unsigned long Length);
	virtual void EndStream();
	virtual void ResetBuffer();
	virtual unsigned long GetBufferedLength() const;
};