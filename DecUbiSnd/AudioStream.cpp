// AudioStream.h : Audio stream decoding abstract class
//

#include "stdafx.h"
#include "AudioStream.h"

CAudioStream::CAudioStream(std::istream& Input, std::streamsize Size) :
	m_Input(Input),
	m_BeginOffset(m_Input.tellg()),
	m_EndOffset(m_BeginOffset+Size)
{
	return;
}

CAudioStream::CAudioStream(std::istream& Input, std::streamoff Offset, std::streamsize Size) :
	m_Input(Input),
	m_BeginOffset(Offset),
	m_EndOffset(m_BeginOffset+Size)
{
	return;
}

CAudioStream::~CAudioStream()
{
	return;
}

unsigned long CAudioStream::RecommendBufferLength() const
{
	return 65536;
}
