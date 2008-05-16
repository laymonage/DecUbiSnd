// Version5.cpp : Convert UbiSoft format version 3 and 5 audio
//

#include "stdafx.h"
#include "Adpcm.h"

/*
// Decompress the file
if(Args.InputSize<2)
{
	std::cout << "Input size not specified." << std::endl;
	return 1;
}
if(!ConvertVersion5(Input, Output, Args.InputSize-1, Args.InputStereo ? 2 : 1))
{
	std::cout << "Problems decompressing the input file." << std::endl;
	return 5;
}

// Set some information
SampleRate=32000;
BitsPerSample=16;
Channels=Args.InputStereo ? 2 : 1;
NumberSamples=(unsigned long)(Args.InputSize-Args.InputHeaderDebug)*2;
*/

// Convert UbiSoft version 3 or 5 audio
bool ConvertVersion5(std::istream& Input, std::ostream& Output, size_t Size, unsigned char Channels)
{
	// Error checking
	bool Return=true;
	if(Size==0)
	{
		return false;
	}
	if(Channels<0 || Channels>2)
	{
		return false;
	}
	if(Channels==0)
	{
		std::cout << "Warning: Automatic channels detection has not yet been implemented. Assuming stereo." << std::endl;
		Channels=2;
	}

	// Determine the header size
	size_t HeaderSize;
	if(Channels==2)
	{
		HeaderSize=68;
	}
	else
	{
		HeaderSize=48;
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
	if(Channels==1)
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
		if(Channels==1)
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
