#include "Pch.h"
#include "PortAudioWrapper.h"

#include <portaudio.h>

#define FRAMES_PER_BUFFER 24000

PortAudioWrapper::PortAudioWrapper()
{
	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		// TODO Handle this error
	}
}

PortAudioWrapper::~PortAudioWrapper()
{
	PaError err = Pa_Terminate();
	if (err != paNoError)
	{
		// TODO Handle this error
	}
}

PortAudioStream* PortAudioWrapper::createStream(unsigned char channels, unsigned int sampleRate) const
{
	PaDeviceIndex device = -1;
	for (PaHostApiIndex i = 0; i < Pa_GetHostApiCount(); i++)
	{
		const PaHostApiInfo& info = *Pa_GetHostApiInfo(i);

		if (info.type == paDirectSound)
		{
			device = info.defaultOutputDevice;
		}
	}

	// Choose the default device if we couldn't find a good one
	//if (device == -1)
		device = Pa_GetDefaultOutputDevice();

	PaStreamParameters params;
	params.device = device;
    params.channelCount = channels;
    params.sampleFormat = paInt16;
    params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultHighOutputLatency;
    params.hostApiSpecificStreamInfo = NULL;

	const PaDeviceInfo* di = Pa_GetDeviceInfo(params.device);
	//wxMessageBox(wxString::Format("Host api: %s", Pa_GetHostApiInfo(di->hostApi)->name), "Device");

	PaStream* stream;
	PaError err = Pa_OpenStream(
		&stream,
		NULL,
		&params,
		sampleRate,
		paFramesPerBufferUnspecified,
		paClipOff,
		NULL,
		NULL);

	if (err != paNoError || !stream)
	{
		// TODO Handle this error
		wxMessageBox("Pa_OpenStream failed.");
		return NULL;
	}

	return new PortAudioStream(stream);
}

PortAudioStream::PortAudioStream(PaStream* stream):
	stream(stream)
{
}

PortAudioStream::~PortAudioStream()
{
	// Stop the stream
	Pa_StopStream(stream);

	// Close the stream
	PaError err = Pa_CloseStream(stream);
	if (err != paNoError)
	{
		// TODO Handle this error
	}
	stream = NULL;
}

bool PortAudioStream::start()
{
	PaError err = Pa_StartStream(stream);
	if (err != paNoError)
	{
		// TODO Handle this error
		wxMessageBox("Pa_StartStream failed.");
		return false;
	}
	return true;
}

bool PortAudioStream::stop()
{
	PaError err = Pa_StopStream(stream);
	if (err != paNoError)
	{
		// TODO Handle this error
		return false;
	}
	return true;
}

bool PortAudioStream::abort()
{
	PaError err = Pa_AbortStream(stream);
	if (err != paNoError)
	{
		// TODO Handle this error
		return false;
	}
	return true;
}

long PortAudioStream::getWriteAvailable()
{
	return Pa_GetStreamWriteAvailable(stream);
}

bool PortAudioStream::write(short* buffer, unsigned int frameCount)
{
	PaError err = Pa_WriteStream(stream, buffer, frameCount);
	if (err != paNoError)
	{
		// TODO Handle this error
		wxMessageBox("Pa_WriteStream failed.");
		return false;
	}
	return true;
}
