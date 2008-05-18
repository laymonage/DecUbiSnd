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
				const std::streamoff ChunkStart=(std::streamoff)CurrentOffset+i;

				// Some assumptions
				unsigned char Char[28];
				bool ChunkValid=false;

				// Read in the characters
				Input.seekg(ChunkStart);
				Input.read((char*)Char, 28);

				// Check the characters
				if(Char[9]==0 && Char[10]==0 && Char[11]==0 && Char[18]<89 && \
					(Char[19]==0 || Char[19]==119) && Char[22]<89 && Char[23]<5)
				{
					ChunkValid=true;
				}

				// Check some other conditions
				if(ChunkValid && (Char[0]==3))
				{
					if(Char[14]!=0 || Char[15]!=10)
					{
						ChunkValid=false;
					}
				}
				else if(ChunkValid && (Char[0]==5))
				{
					if(Char[14]!=10 || Char[15]!=0)
					{
						ChunkValid=false;
					}
				}

				// If the 
				if(ChunkValid)
				{
					// The file is valid so far, so return success
					Input.seekg(ChunkStart);
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

		if(BytesRead<48)
		{
			// Skip to the next file; this one is too small
			// The next file cannot start at the next byte
			Input.seekg(29, std::ios_base::cur);
			Found=DoScan(Input, EndOffset, BytesRead);
			continue;
		}

		std::cout << (int)ChunkOffset << "\t" << (int)BytesRead;
		NumberFound++;

		/*unsigned char Char[36];
		std::streamoff Prev=Input.tellg();
		Input.seekg(ChunkOffset);
		Input.read((char*)Char, 36);
		std::cout << "\t";
		for(unsigned long j=0;j<36;j++)
		{
			std::cout << (int)Char[j] << "\t";
		}
		Input.seekg(Prev);*/

		std::cout << std::endl;
	}
	std::cerr << std::endl << "Found: " << NumberFound << std::endl;
	return true;
}
