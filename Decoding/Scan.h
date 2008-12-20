// Scan.h : Scan for UbiSoft format audio in files
//

#pragma once

struct SFound
{
	std::streamoff Offset;
	std::streamsize Size;
};

class wxProgressDialog;

// List the UbiSoft format audio chunks in the file
bool ScanAndList(wxProgressDialog* Progress, std::istream& Input, std::vector<SFound>& FoundList, std::streamoff EndOffset);
