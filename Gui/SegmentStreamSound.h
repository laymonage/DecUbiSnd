/*
 SegmentStreamSound.h : A segment stream sound
*/

#pragma once

#include <wx/mmedia/sndfile.h>
#include <wx/wfstream.h> // TODO: Delete

namespace NDecFunc
{
	class CSegmentStream;
};

namespace NDecGui
{
	class CSegmentStreamSound : public wxSoundFileStream
	{
	protected:
		NDecFunc::CSegmentStream& m_Stream;
		bool m_Looping;
		unsigned long m_ExtraSamples;
		wxFFileInputStream m_File; // TODO: Delete

	protected:
		virtual bool PrepareToPlay();
		virtual bool PrepareToRecord(wxUint32 Time);
		virtual bool FinishRecording();
		virtual bool RepositionStream(wxUint32 Position);
		virtual wxUint32 GetData(void *Buffer, wxUint32 Size);
		virtual wxUint32 PutData(const void *Buffer, wxUint32 Size);

	public:
		CSegmentStreamSound(NDecFunc::CSegmentStream& Stream, wxSoundStream& IoStream);
		virtual ~CSegmentStreamSound();

		virtual bool CanRead();
		virtual wxString GetCodecName() const;
		virtual void SetLooping(bool Looping);
		virtual bool IsLooping() const;
	};
};
