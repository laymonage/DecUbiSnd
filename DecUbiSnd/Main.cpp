// Main.cpp : Defines the entry point for the console application
//

#include "stdafx.h"
#include "Type3.h"
#include "WaveWriter.h"

// The arguments sent to the application
struct SArguments
{
	SArguments() :
		ShowBanner(true),
		ShowUsage(false),
		InputFilename(""),
		InputOffset(0),
		InputSize(0),
		InputHeaderDebug(72),
		InputStereo(false),
		InputTypeForce(0),
		OutputFilename(""),
		OuputWaveFile(false),
		OutputSampleRate(0)
	{
	};

	bool ShowBanner;
	bool ShowUsage;
	std::string InputFilename;
	size_t InputOffset;
	size_t InputSize;
	size_t InputHeaderDebug;
	bool InputStereo;
	unsigned char InputTypeForce;
	std::string OutputFilename;
	bool OuputWaveFile;
	unsigned long OutputSampleRate;
};

// Parse the arguments sent to the program
bool ParseArguments(SArguments& Args, unsigned long Argc, _TCHAR* Argv[])
{
	// Go through each of the arguments sent
	for(unsigned long i=1;i<Argc;)
	{
		std::string Arg(Argv[i++]);
		
		// Check it
		if(Arg=="-b" || Arg=="--no-banner")
		{
			Args.ShowBanner=false;
		}
		else if(Arg=="-i" || Arg=="--offset")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputOffset=atol(Argv[i++]);
		}
		else if(Arg=="-s" || Arg=="--size")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputSize=atol(Argv[i++]);
		}
		else if(Arg=="--header-size")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputHeaderDebug=atol(Argv[i++]);
		}
		else if(Arg=="-t" || Arg=="--stereo")
		{
			Args.InputStereo=true;
		}
		else if(Arg=="-m" || Arg=="--mono")
		{
			Args.InputStereo=false;
		}
		else if(Arg=="--input-type")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputTypeForce=atoi(Argv[i++]);
		}
		else if(Arg=="-o" || Arg=="--output")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.OutputFilename=std::string(Argv[i++]);
		}
		else if(Arg=="-w" || Arg=="--wave")
		{
			Args.OuputWaveFile=true;
		}
		else if(Arg=="-r" || Arg=="--raw")
		{
			Args.OuputWaveFile=false;
		}
		else if(Arg=="--sample-rate")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.OutputSampleRate=atoi(Argv[i++]);
		}
		else
		{
			Args.InputFilename=Arg;
		}
	}
	return true;
}

int _tmain(int Argc, _TCHAR* Argv[])
{
	// Parse the arguments
	SArguments Args;
	if(Argc==1)
	{
		Args.ShowUsage=true;
	}
	bool ArgParse=ParseArguments(Args, Argc, Argv);

	// Display banner
	if(Args.ShowBanner)
	{
		std::cout << "Sound/Music Decoder for UBISOFT Formats" << std::endl << std::endl;
	}

	// Display usage
	if(Args.ShowUsage)
	{
		std::cout << "Usage: DecUbiSnd InputFilename [Options]" << std::endl;
		std::cout << "  -o, --output File     Specify the output filename" << std::endl;
		std::cout << "  -w, --wave            Output a standard wave file (.wav)" << std::endl;
		std::cout << "  -r, --raw             Output raw data (no header)" << std::endl;
		std::cout << "  -i, --offset Number   Specify the offset to begin decoding" << std::endl;
		std::cout << "  -s, --size Number     Specify the number of bytes to decode" << std::endl;
		std::cout << "  -t, --stereo          The data is stereo" << std::endl;
		std::cout << "  -m, --mono            The data is mono" << std::endl;
		std::cout << std::endl;
		std::cout << "  --header-size Number  Specify the header size" << std::endl;
		std::cout << "  --input-type Type     Force it to use the decoder for Type (3 or 5)" << std::endl;
		std::cout << "  --sample-rate Rate    Force a specific sampling rate" << std::endl;
		std::cout << std::endl;
	}

	// Check for errors
	if(!ArgParse)
	{
		std::cout << "Parameters are not valid." << std::endl;
		return 1;
	}

	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cout << "Input file not specified." << std::endl;
		return 1;
	}
	if(Args.OutputFilename=="")
	{
		std::cout << "Output file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cout << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// Open the output stream
	std::ofstream Output;
	Output.open(Args.OutputFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		std::cout << "Unable to open output file '" << Args.OutputFilename << "'." << std::endl;
		return 3;
	}

	// Get the type
	unsigned char InputType=Args.InputTypeForce;
	Input.seekg((std::streamoff)Args.InputOffset);
	if(InputType==0)
	{
		InputType=Input.get();
	}
	else
	{
		Input.get();
	}

	// Prepare for the wave header, if required
	unsigned long SampleRate;
	unsigned char BitsPerSample;
	unsigned char Channels;
	unsigned long NumberSamples=0;
	if(Args.OuputWaveFile)
	{
		PrepareWaveHeader(Output);
	}

	// Decompress the audio
	if(InputType==3)
	{
		// Decompress the file
		if(Args.InputSize<2)
		{
			std::cout << "Input size not specified." << std::endl;
			return 1;
		}
		if(!ConvertType3(Input, Output, Args.InputSize-1, Args.InputStereo, Args.InputHeaderDebug-1))
		{
			std::cout << "Problems decompressing the input file." << std::endl;
			return 5;
		}

		// Set some information
		SampleRate=32000;
		BitsPerSample=16;
		Channels=Args.InputStereo ? 2 : 1;
		NumberSamples=(unsigned long)(Args.InputSize-Args.InputHeaderDebug)*2;
	}
	else
	{
		std::cout << "The input file is not a supported format. ";
		std::cout << "Use --input-type to force a specific format." << std::endl;
		return 4;
	}

	// Fix the wave header
	if(Args.OutputSampleRate!=0)
	{
		SampleRate=Args.OutputSampleRate;
	}
	if(Args.OuputWaveFile)
	{
		Output.seekp(0);
		WriteWaveHeader(Output, SampleRate, BitsPerSample, Channels, NumberSamples);
	}

	// Finish up
	Input.close();
	Output.close();
	return 0;
}
