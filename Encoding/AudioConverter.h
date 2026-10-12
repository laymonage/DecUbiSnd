#pragma once

#include <string>

enum EAudioFitMode
{
	EAFM_TRIM_PAD,
	EAFM_TIME_STRETCH
};

void ConvertAudioToSc1ReplacementWave(
	const std::wstring& InputPath,
	const std::wstring& OutputPath,
	unsigned long SampleRate,
	unsigned long SampleCount,
	double StartSeconds,
	EAudioFitMode FitMode);
