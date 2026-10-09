#pragma once

#include <string>

struct SSc1SegmentInfo
{
	unsigned long SampleRate;
	unsigned long SampleCount;
	unsigned long long SegmentSize;
};

SSc1SegmentInfo InspectSc1AdpcmSegment(
	const std::string& BankFilename,
	unsigned long long Offset);

SSc1SegmentInfo ReplaceSc1AdpcmSegment(
	const std::string& BankFilename,
	const std::string& WaveFilename,
	const std::string& OutputFilename,
	unsigned long long Offset);
