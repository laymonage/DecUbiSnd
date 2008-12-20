/*
 SegmentStream.h : A stream of concatenated segments
*/

#pragma once

class CFileDataStream;
class CAudioStream;

namespace NDecFunc
{
	class CSegment;

	class CSegmentStream : protected std::vector<CSegment*>
	{
	protected:
		unsigned long m_CurrentSegment;
		std::ifstream m_InputFile;
		CFileDataStream* m_InputStream;
		CAudioStream* m_AudioStream;

	protected:
		virtual bool InitializeAudioSegment(CSegment& Segment);
		virtual void ReleaseAudioSegment();

	public:
		CSegmentStream();
		virtual ~CSegmentStream();

		virtual void Add(const CSegment* Segment);
		virtual CSegment* Get(unsigned long Index);
		virtual void Clear();
		virtual unsigned long GetCount() const;
		virtual void Restart();
		virtual unsigned long GetProgess(unsigned long Max) const;
		virtual CSegment* GetCurrentSegment() const;
		virtual bool Decode(short* Buffer, unsigned long& NumberSamples);
	};
};
