/*
 SegmentStreamSound.h : A segment stream sound
*/

#pragma once

#include <wx/timer.h>
#include <windows.h>
#include <mmsystem.h>

namespace NDecFunc
{
	class CSegmentStream;
};

namespace NDecGui
{
	// Plays a segment stream through the Windows waveOut API. Buffers are
	// refilled from a wxTimer, so all decoding happens on the GUI thread.
	class CSegmentStreamSound : public wxTimer
	{
	protected:
		static const unsigned long NumberBuffers=8;
		static const unsigned long BufferMilliseconds=100;

		NDecFunc::CSegmentStream& m_Stream;
		bool m_Looping;
		unsigned long m_ExtraSamples;
		HWAVEOUT m_WaveOut;
		WAVEHDR m_Headers[NumberBuffers];
		std::vector<short> m_Buffers[NumberBuffers];
		bool m_Playing;
		bool m_EndOfStream;

	protected:
		unsigned long GetData(short* Buffer, unsigned long NumberSamples);
		bool Fill(unsigned long Index);
		void Close();

	public:
		CSegmentStreamSound(NDecFunc::CSegmentStream& Stream);
		virtual ~CSegmentStreamSound();

		virtual bool Play();
		virtual void Stop();
		virtual bool IsStopped() const;
		virtual void SetLooping(bool Looping);
		virtual bool IsLooping() const;
		virtual void Notify();
	};
};
