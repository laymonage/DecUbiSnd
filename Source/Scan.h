// Scan.h : Scan for UbiSoft format audio in files
//

#pragma once

// List the UbiSoft format audio chunks in the file
bool ScanAndList(std::istream& Input, std::streamoff EndOffset);
