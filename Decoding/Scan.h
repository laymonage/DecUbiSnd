// Scan.h : Scan for UbiSoft format audio in files
//

#pragma once

#include <iosfwd>
#include <vector>

struct SFound
{
	std::streamoff Offset;
	std::streamsize Size;
};

typedef bool (*TScanProgressCallback)(void* Context, unsigned long Value);

// List the UbiSoft format audio chunks in the file
bool ScanAndList(std::istream& Input, std::streamoff EndOffset);
bool ScanAndList(std::istream& Input, std::vector<SFound>& FoundList,
	std::streamoff EndOffset, TScanProgressCallback ProgressCallback,
	void* ProgressContext);
