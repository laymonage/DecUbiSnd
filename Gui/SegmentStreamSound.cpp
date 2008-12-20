/*
 SegmentStreamSound.cpp : A segment stream sound
*/

#include "Pch.h"

#include <wx/apptrait.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/mmedia/sndpcm.h>

#include "Gui/App.h"
#include "Gui/SegmentStreamSound.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"

// NDecGui Implementation
NDecGui::CSegmentStreamSound::CSegmentStreamSound(NDecFunc::CSegmentStream& Stream, wxSoundStream& IoStream) :
	m_File(wxGetApp().GetTraits()->GetStandardPaths().GetExecutablePath(), wxT("rb")),
	wxSoundFileStream(m_File, IoStream),
	m_Stream(Stream),
	m_Looping(false)
{
	return;
}

NDecGui::CSegmentStreamSound::~CSegmentStreamSound()
{
	return;
}

bool NDecGui::CSegmentStreamSound::CanRead()
{
	// We can always read
	return true;
}

wxString NDecGui::CSegmentStreamSound::GetCodecName() const
{
	return wxT("SegmentStream: UbiSoft Audio Decoder");
}

void NDecGui::CSegmentStreamSound::SetLooping(bool Looping)
{
	m_Looping=Looping;
	return;
}

bool NDecGui::CSegmentStreamSound::IsLooping() const
{
	return m_Looping;
}

bool NDecGui::CSegmentStreamSound::PrepareToPlay()
{
	// Get the next segment
	wxSoundFormatPcm SoundFormat;
	NDecFunc::CSegment* Segment=m_Stream.GetCurrentSegment();
	if(!Segment)
	{
		return false;
	}

	// Set up the sound format
	SoundFormat.SetSampleRate(Segment->GetSampleRate());
	SoundFormat.SetBPS(16);
	SoundFormat.SetChannels(Segment->GetChannels());
	SoundFormat.Signed(true);
	SoundFormat.SetOrder(wxLITTLE_ENDIAN);

	if(!SetSoundFormat(SoundFormat))
	{
		return false;
	}

	FinishPreparation(GetBestSize());
	return true;
}

bool NDecGui::CSegmentStreamSound::PrepareToRecord(wxUint32 Time)
{
	// We cannot record
	return false;
}

bool NDecGui::CSegmentStreamSound::FinishRecording()
{
	// We cannot record
	return false;
}

bool NDecGui::CSegmentStreamSound::RepositionStream(wxUint32 Position)
{
	// We cannot seek
	return false;
}

wxUint32 NDecGui::CSegmentStreamSound::GetData(void *Buffer, wxUint32 Size)
{
	// Decode some samples
	unsigned long SampleToDecode=Size/2;
	unsigned long SamplesDecoded=SampleToDecode;
	if(!m_Stream.Decode((short*)Buffer, SamplesDecoded))
	{
		// End the stream right here
		m_bytes_left=0;
		return SamplesDecoded*2;
	}

	// If we are looping, look for more
	if(m_Looping && SamplesDecoded<SampleToDecode)
	{
		unsigned long NewDecoded=SampleToDecode-SamplesDecoded;
		m_Stream.Restart();
		if(!m_Stream.Decode((short*)Buffer+SamplesDecoded, NewDecoded))
		{
			// End the stream right here
			m_bytes_left=0;
			return SamplesDecoded*2;
		}
		SamplesDecoded+=NewDecoded;
	}

	// Just a little hack
	if(SamplesDecoded)
	{
		m_bytes_left+=SamplesDecoded*2;
	}
	else
	{
		m_bytes_left=0;
	}
	return SamplesDecoded*2;
}

wxUint32 NDecGui::CSegmentStreamSound::PutData(const void *Buffer, wxUint32 Size)
{
	// We cannot write data to file
	return 0;
}
