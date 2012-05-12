#pragma once
#include <portaudio.h>

class PortAudioStream;

class PortAudioWrapper
{
public:
	PortAudioWrapper();
	~PortAudioWrapper();
	PortAudioStream* createStream(unsigned char channels, unsigned int sampleRate) const;
};

class PortAudioStream
{
public:
	PortAudioStream(PaStream* stream);
	~PortAudioStream();
	bool start();
	bool stop();
	bool abort();
	long getWriteAvailable();
	bool write(short* buffer, unsigned int frameCount);

private:
	PaStream* stream;
};
