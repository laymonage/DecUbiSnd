// Type3.cpp : Decompresses UbiSoft format type 3 audio
//

#include "stdafx.h"
#include "Adpcm.h"

// Convert UbiSoft format type 3 audio
bool ConvertType3(std::istream& Input, std::ostream& Output, size_t Size, bool Stereo, size_t HeaderSize)
{
	// Error checking
	bool Return=true;
	if(Size==0)
	{
		return false;
	}

	// Create the buffers
	unsigned long InputBufferLength=65535;
	unsigned long OutputBufferLength=InputBufferLength*2;
	char* InputBuffer=new char[InputBufferLength];
	short* OutputBuffer=new short[OutputBufferLength];
	size_t BytesLeft=Size-HeaderSize;

	// Decompression parameters
	SAdpcmMonoParam MonoParam;
	SAdpcmStereoParam StereoParam;
	MonoParam.InputBuffer=reinterpret_cast<unsigned char*>(InputBuffer);
	StereoParam.InputBuffer=reinterpret_cast<unsigned char*>(InputBuffer);
	MonoParam.OutputBuffer=OutputBuffer;
	StereoParam.OutputBuffer=OutputBuffer;

	// Read the header
	if(!Stereo)
	{
		Input.seekg(15, std::ios_base::cur);
		Input.read((char*)&MonoParam.FirstSample, 2);
		Input.read((char*)&MonoParam.FirstIndex, 1);
		Input.seekg(4, std::ios_base::cur);
	}
	else
	{
		Input.seekg(15, std::ios_base::cur);
		Input.read((char*)&StereoParam.FirstLeftSample, 2);
		Input.read((char*)&StereoParam.FirstLeftIndex, 1);
		Input.seekg(1, std::ios_base::cur);
		Input.read((char*)&StereoParam.FirstRightSample, 2);
		Input.read((char*)&StereoParam.FirstRightIndex, 1);
	}
	Input.seekg((unsigned long)HeaderSize-22, std::ios_base::cur);

	// The read loop
	do
	{
		unsigned long InputLength;

		// Check the buffer
		if(BytesLeft>InputBufferLength)
		{
			InputLength=InputBufferLength;
		}
		else
		{
			InputLength=(unsigned long)BytesLeft;
		}

		// Read the data
		Input.read(InputBuffer, InputLength);

		// Decompress the data
		if(!Stereo)
		{
			MonoParam.InputLength=InputLength;
			if(!DecompressMonoAdpcm(&MonoParam))
			{
				Return=false;
				break;
			}
		}
		else
		{
			StereoParam.InputLength=InputLength;
			if(!DecompressStereoAdpcm(&StereoParam))
			{
				Return=false;
				break;
			}
		}

		// Write the output
		Output.write((char*)OutputBuffer, InputLength*4);

		// Update the number of bytes left
		BytesLeft-=InputLength;
	} while(BytesLeft);

	// Finish up
	delete [] InputBuffer;
	delete [] OutputBuffer;
	return Return;
}
