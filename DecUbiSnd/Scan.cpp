// Scan.cpp : Scan for UbiSoft format audio in files
//

#include "stdafx.h"
#include "Scan.h"

// Do the actual scanning, to figure out roughly when the next audio is
static bool DoScan(std::istream& Input, size_t EndOffset, size_t& BytesRead)
{
	// Just check first
	if((size_t)Input.tellg()>=EndOffset)
	{
		return false;
	}

	// Some variables
	const size_t InputBufferLength=65535;
	unsigned char* Buffer=new unsigned char[InputBufferLength];
	size_t BytesLeft=EndOffset-Input.tellg();
	size_t StartOffset=Input.tellg();

	// The scanning loop
	while((size_t)Input.tellg()<EndOffset)
	{
		// Calculate the amount of data that needs to be read
		size_t NextRead;
		size_t CurrentOffset=Input.tellg();
		if(BytesLeft>InputBufferLength)
		{
			NextRead=InputBufferLength;
		}
		else
		{
			NextRead=BytesLeft;
		}

		// This should not happen, but we'll check for it anyways
		if(!NextRead)
		{
			break;
		}

		// Read the data
		Input.read((char*)Buffer, (std::streamsize)NextRead);
		BytesLeft-=NextRead;

		// Process the data in a for loop
		size_t OffsetReset=Input.tellg();
		for(unsigned long i=0;i<(unsigned long)NextRead;i++)
		{
			if(Buffer[i]==3 || Buffer[i]==5)
			{
				// Store some variables
				const size_t ChunkStart=CurrentOffset+i;

				// Some assumptions
				unsigned char Char14;
				unsigned char Char15;
				unsigned char Char18;
				unsigned char Char19;
				unsigned char Char22;
				unsigned char Char23;

				// Read in the characters
				Input.seekg((std::streamoff)ChunkStart+14);
				Char14=Input.get();
				Char15=Input.get();
				Input.seekg(2, std::ios_base::cur);
				Char18=Input.get();
				Char19=Input.get();
				Input.seekg(2, std::ios_base::cur);
				Char22=Input.get();
				Char23=Input.get();

				// Check the characters
				if(/*Char14==0 && Char15==10 &&*/ Char18<89 && (Char19==0 || Char19==119) && \
					Char22<89 && Char23<5) // Not so sure about last condition
				{
					// The file is valid so far, so return success
					Input.seekg((std::streamoff)ChunkStart);
					BytesRead=ChunkStart-StartOffset;
					delete [] Buffer;
					return true;
				}

				// If this was just a false alarm
				Input.seekg((std::streamoff)(OffsetReset));
			}
		}
	}

	// Clean up
	delete [] Buffer;
	return false;
}

// List the UbiSoft format audio chunks in the file
bool ScanAndList(std::istream& Input, size_t EndOffset)
{
	size_t BytesRead;
	unsigned long NumberFound=0;
	bool Found=DoScan(Input, EndOffset, BytesRead);

	while(Found)
	{
		size_t ChunkOffset=Input.tellg();
		Input.seekg(28, std::ios_base::cur);
		Found=DoScan(Input, EndOffset, BytesRead);
		if(Found)
		{
			BytesRead+=28;
		}
		else
		{
			BytesRead=EndOffset-ChunkOffset;
		}

		//std::cout << (int)ChunkOffset << ", " << (int)BytesRead << std::endl;
		std::cout << "DecUbiSnd -D \"\" -i " << (int)ChunkOffset << " -s " << \
			(int)BytesRead << " --stereo -w -o __Scan" << NumberFound << ".wav" << \
			std::endl;
		NumberFound++;
	}
	//std::cout << std::endl << "Found: " << NumberFound << std::endl;
	return false;
}
