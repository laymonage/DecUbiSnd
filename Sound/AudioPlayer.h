#pragma once

#include <wx/timer.h>
#include <wx/stopwatch.h>
#include <wx/thread.h>

#include "PortAudioWrapper.h"

namespace NDecFunc
{
	class CSegmentStream;
};

using NDecFunc::CSegmentStream;

class AudioPlayer;

/*class AudioPlayerTimer : public wxTimer
{
public:
	AudioPlayerTimer(AudioPlayer& player);
	virtual void Notify();

private:
	AudioPlayer& player;
};*/

class AudioPlayerThread : public wxThread
{
public:
	AudioPlayerThread(volatile bool& playing, volatile bool& loop, wxMutex& mutex, wxCondition& cond, CSegmentStream* inputStream, PortAudioStream* audioStream);
	virtual ExitCode Entry();

private:
	volatile bool& playing;
	volatile bool& loop;
	wxMutex& mutex;
	wxCondition& cond;
	CSegmentStream* inputStream;
	PortAudioStream* audioStream;

	char channels;
	int sampleRate;
	short* decodeBuffer;
	unsigned int decodeBufferCount;

	// Functions assume lock is held
	bool writeStream();
};

class AudioPlayer
{
public:
	AudioPlayer(wxEvtHandler* owner, int timerId = -1);
	~AudioPlayer();
	bool play(NDecFunc::CSegmentStream* inputStream);
	void setLoop(bool loop);
	void stop();
	void finish();
	bool isPlaying() const;
	long getPlayingTime() const;

	/**
	 * Call for every timer event. The function will return true if the stream
	 * finished playing, false otherwise.
	 */
	bool update();

private:
	PortAudioWrapper portAudio;
	wxTimer audioTimer;
	wxStopWatch playingTime;

	AudioPlayerThread* audioThread;

	// Shared state with the audio thread
	wxMutex stopMutex;
	wxCondition stopCond;

	volatile bool playing;
	volatile bool loop;
};
