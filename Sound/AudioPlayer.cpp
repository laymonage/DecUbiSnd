#include "Pch.h"
#include "AudioPlayer.h"
#include "../Functionality/SegmentStream.h"
#include "../Functionality/Segment.h"

using NDecFunc::CSegment;
using NDecFunc::CSegmentStream;

AudioPlayer::AudioPlayer(wxEvtHandler* owner, int timerId):
	audioTimer(owner, timerId),
	audioThread(NULL),
	loop(false),
	stopCond(stopMutex),
	playing(false)
{
}

AudioPlayer::~AudioPlayer()
{
	stop();
}

bool AudioPlayer::play(NDecFunc::CSegmentStream* inputStream)
{
	assert(inputStream);
	stop();

	// Get some info about the stream
	if (inputStream->GetCount() < 1)
		return false;

	const NDecFunc::CSegment& segment = *inputStream->GetCurrentSegment();
	int sampleRate = segment.GetSampleRate();
	char channels = segment.GetChannels();

	// Better safe than segfaulted
	if (sampleRate < 1 || channels < 1)
		return false;

	// Start a portaudio stream, locking the mutext because PortAudio isn't thread
	// safe. We might accidentally be creating a stream at the same time as
	// destroying. (This lock is very important!)
	stopMutex.Lock();
	PortAudioStream* audioStream = portAudio.createStream(channels, sampleRate);
	audioStream->start();
	stopMutex.Unlock();

	// Set state to playing
	playing = true;

	// New thread
	audioThread = new AudioPlayerThread(playing, loop, stopMutex, stopCond, inputStream, audioStream);
	audioThread->Create();
	audioThread->SetPriority(65);
	audioThread->Run();

	// Start timer
	audioTimer.Start(50);
	playingTime.Start();

	wxLogDebug("Started playing at %d hz with %d channels", sampleRate, channels);
	return true;
}

void AudioPlayer::setLoop(bool loop)
{
	this->loop = loop;
}

void AudioPlayer::stop()
{
	wxMutexLocker locker(stopMutex);
	stopCond.Broadcast();

	audioTimer.Stop();
	playingTime.Pause();
}

void AudioPlayer::finish()
{
	stop();
}

bool AudioPlayer::isPlaying() const
{
	return playing;
}

long AudioPlayer::getPlayingTime() const
{
	if (isPlaying())
		return playingTime.Time();
	else
		return 0;
}

bool AudioPlayer::update()
{
	if (!isPlaying())
	{
		audioTimer.Stop();
		playingTime.Pause();
	}
	return false;
}

AudioPlayerThread::AudioPlayerThread(volatile bool& playing, volatile bool& loop, wxMutex& mutex, wxCondition& cond, CSegmentStream* inputStream, PortAudioStream* audioStream):
	playing(playing),
	loop(loop),
	mutex(mutex),
	cond(cond),
	inputStream(inputStream),
	audioStream(audioStream),
	channels(0),
	sampleRate(0),
	decodeBuffer(NULL),
	decodeBufferCount(0)
{
	assert(inputStream);
	assert(audioStream);
}

wxThread::ExitCode AudioPlayerThread::Entry()
{
	assert(audioStream);
	assert(inputStream);

	// I think I'll get away with locking this once at the beginning
	wxMutexLocker locker(mutex);

	playing = true;

	// Fetch some info about the stream
	const CSegment& segment = *inputStream->GetCurrentSegment();
	sampleRate = segment.GetSampleRate();
	channels = segment.GetChannels();

	// Better safe than segfaulted
	if (sampleRate < 1 || channels < 1)
		return false;

	// Create a buffer large enough to hold 4 seconds of data
	decodeBufferCount = 4 * sampleRate * channels;
	decodeBuffer = new short[decodeBufferCount];

	// Loop to continously fill the buffer with data
	try
	{
		while (playing)
		{
			// Wait for the condition to stop for 30 ms
			if (cond.WaitTimeout(30) == wxCOND_NO_ERROR)
			{
				playing = false;
				audioStream->abort();
				break;
			}

			// Fill the buffer
			if (writeStream())
				break;
		}
	}
	catch (...)
	{
		wxLogError("Audio playing thread exceptioned");
	}

	// Probably not needed
	playing = false;
	audioStream->stop(); 

	// Delete any allocated buffers
	delete audioStream;
	audioStream = NULL;
	delete inputStream;
	inputStream = NULL;
	delete decodeBuffer;
	decodeBuffer = NULL;
	decodeBufferCount = 0;
	return 0;
}

bool AudioPlayerThread::writeStream()
{
	long available = audioStream->getWriteAvailable();

	if (available > 0)
	{
		// Decode as much as we can
		const unsigned long maxCount = std::min<long>(available * channels, decodeBufferCount);
		unsigned long sampleCount = maxCount;

		if (!inputStream->Decode(decodeBuffer, sampleCount))
		{
			// TODO What does it mean when Decode returns false?
		}

		wxLogDebug("APT: Available: %d; decoded: %d", available * channels, sampleCount);

		// Are we done yet?
		if (sampleCount < maxCount && loop)
		{
			// Put more samples in the buffer
			short* buffer = decodeBuffer + sampleCount;
			unsigned long count = maxCount - sampleCount;

			inputStream->Restart();

			if (!inputStream->Decode(buffer, count))
			{
				// TODO What does it mean when Decode returns false?
			}

			sampleCount += count;
		}
		else if (sampleCount < 1)
		{
			playing = false;
			return true;
		}

		// The write function takes its argument in sample frames, whereas the other stuff doesn't
		audioStream->write(decodeBuffer, sampleCount / channels);
	}
	return false;
}