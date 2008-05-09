// Type3.h : Convert UbiSoft format type 3 audio
//

// Convert UbiSoft format type 3 audio
bool ConvertType3(std::istream& Input, std::ostream& Output, size_t Size, bool Stereo, size_t HeaderSize=72);
