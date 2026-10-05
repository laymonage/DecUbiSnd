/*
 Segment.h : A segment definition
*/

#pragma once

class CAudioStream;
class CLayeredAudioStream;
#include "Decoding/UbiFormats.h"

namespace NDecGui
{
	class CSegmentsListView;
};

namespace NDecFunc
{
	class CSegment
	{
	protected:
		std::string m_Filename;
		std::streamoff m_Offset;
		std::streamsize m_Size;
		EUbiFormat m_Type;
		unsigned char m_Channels;
		unsigned long m_SampleRate;
		std::vector<unsigned long> m_Layers;
		NDecGui::CSegmentsListView* m_ListView;
		unsigned long m_ListViewIndex;

	public:
		CSegment();
		CSegment(std::string Filename, std::streamoff Offset, std::streamsize Size, \
			EUbiFormat Type=EUF_NULL, unsigned char Channels=0, unsigned long SampleRate=0);
		CSegment(const CSegment& Object);
		virtual ~CSegment();
		//virtual operator=

		virtual void SetFilename(std::string Filename);
		virtual std::string GetFilename() const;
		virtual void SetOffset(std::streamoff Offset);
		virtual std::streamoff GetOffset() const;
		virtual void SetSize(std::streamsize Size);
		virtual std::streamsize GetSize() const;
		virtual void SetType(EUbiFormat Type);
		virtual EUbiFormat GetType() const;
		virtual void SetChannels(unsigned char Channels);
		virtual unsigned char GetChannels() const;
		virtual void SetSampleRate(unsigned long SampleRate);
		virtual unsigned long GetSampleRate() const;
		virtual std::vector<unsigned long>& GetLayers();
		virtual const std::vector<unsigned long>& GetLayers() const;
		virtual void SetListView(NDecGui::CSegmentsListView* ListView);
		virtual NDecGui::CSegmentsListView* GetListView() const;
		virtual void SetListViewIndex(unsigned long Index);
		virtual unsigned long GetListViewIndex() const;
	};
};
