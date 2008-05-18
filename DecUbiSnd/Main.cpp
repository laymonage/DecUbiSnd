// Main.cpp : Defines the entry point for the console application
//

#include "stdafx.h"
#include "Version5Stream.h"
#include "Scan.h"
#include "SegmentParser.h"
#include "WaveWriter.h"

// The action to be taken
enum EAction
{
	EACT_DECODE,
	EACT_SCAN
};

// The arguments sent to the application
struct SArguments
{
	SArguments() :
		Action(EACT_DECODE),
		ShowBanner(true),
		ShowUsage(false),
		InputFilename(""),
		InputOffset(0),
		InputSize(0),
		InputHeaderDebug(72),
		InputStereo(false),
		InputTypeForce(0),
		OutputFilename(""),
		OuputWaveFile(true),
		OutputSampleRate(0),
		SegmentFilename("")
	{
	};

	EAction Action;
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
	std::string SegmentFilename;
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
		else if(Arg=="-D" || Arg=="--decode")
		{
			Args.Action=EACT_DECODE;
		}
		else if(Arg=="-S" || Arg=="--scan")
		{
			Args.Action=EACT_SCAN;
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
		else if(Arg=="-g" || Arg=="--segments")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.SegmentFilename=std::string(Argv[i++]);
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

int StreamDecoder(CAudioStream& Stream, std::ostream& Output, unsigned long& DecodedSamples)
{
	// Create the buffers
	unsigned long OutputBufferLength=Stream.RecommendBufferLength();
	short* OutputBuffer=new short[OutputBufferLength];
	DecodedSamples=0;

	// Do the loop
	while(true)
	{
		// Decode some
		unsigned long NumberSamples=OutputBufferLength;
		if(!Stream.Decode(OutputBuffer, NumberSamples))
		{
			delete [] OutputBuffer;
			return 5;
		}

		// Check if any was decoded
		if(NumberSamples==0)
		{
			break;
		}

		// Write it to the output stream
		DecodedSamples+=NumberSamples;
		Output.write((char*)OutputBuffer, NumberSamples*2);
	}

	// Clean up
	delete [] OutputBuffer;
	return 0;
}

int Decode(SArguments& Args)
{
	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cerr << "Input file not specified." << std::endl;
		return 1;
	}
	if(Args.OutputFilename=="")
	{
		std::cerr << "Output file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cerr << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// Open the output stream
	std::ofstream Output;
	Output.open(Args.OutputFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		std::cerr << "Unable to open output file '" << Args.OutputFilename << "'." << std::endl;
		return 3;
	}

	// Open the segment file stream and parse the segments
	CSegmentParser Segments;
	if(!Args.SegmentFilename.empty())
	{
		std::ifstream SegmentFile;
		SegmentFile.open(Args.SegmentFilename.c_str(), std::ios_base::in);
		if(!SegmentFile.is_open())
		{
			std::cerr << "Unable to open segment definition file '" << Args.SegmentFilename << "'." << std::endl;
			return 104;
		}
		if(!Segments.Parse(SegmentFile))
		{
			SegmentFile.close();
			std::cerr << "Segment file parsing failed." << std::endl;
			std::cerr << Segments.GetParserMessage() << std::endl;
			return 105;
		}
		SegmentFile.close();

		// TODO: Give a little hint about the filename not being right?
	}

	// Check the segments
	if(Segments.GetSegmentCount()<1)
	{
		CSegment Seg(Args.InputOffset, Args.InputSize);
		Segments.AppendSegment(Seg);
	}

	// Prepare for the wave header, if required
	unsigned long SampleRate;
	unsigned char BitsPerSample;
	unsigned char Channels=2;
	unsigned long NumberSamples=0;
	if(Args.OuputWaveFile)
	{
		PrepareWaveHeader(Output);
	}

	for(unsigned long i=0;i<Segments.GetSegmentCount();i++)
	{
		// Get the current segment
		CSegment Segment(Segments.GetSegment(i));
		unsigned long LocalNumberSamples=0;
		if(Segment.GetSkip())
		{
			continue;
		}

		// Get the type
		unsigned char InputType=Args.InputTypeForce;
		Input.seekg(Segment.GetOffset());
		if(InputType==0)
		{
			InputType=Input.get();
		}
		else
		{
			Input.get();
		}

		// Decompress the audio
		if(InputType==3 || InputType==5)
		{
			// Decode the stream
			CVersion5Stream Stream(Input, Segment.GetOffset(), Segment.GetSize());
			if(!Stream.InitHeader(Args.InputStereo ? 2 : 1, Args.InputTypeForce))
			{
				std::cerr << "Problems initializing the header." << std::endl;
				continue;
			}
			if(StreamDecoder(Stream, Output, LocalNumberSamples))
			{
				std::cerr << "Problems decompressing the input file." << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else
		{
			std::cerr << "The input file is not a supported format. ";
			std::cerr << "Use --input-type to force a specific format." << std::endl;
			continue;
		}
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

int Scan(SArguments& Args)
{
	// Print a warning message
	std::cerr << "Warning: The scan feature is experimental. Use it at your own risk." << std::endl;

	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cerr << "Input file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cerr << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// Open the output stream, if required
	// TODO: This

	// TODO: Scan it
	Input.seekg((std::streamoff)Args.InputOffset);
	ScanAndList(Input, Args.InputOffset+Args.InputSize);

	// Finish up
	Input.close();
	return 0;
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
		std::cerr << "Sound/Music Decoder for UBISOFT Formats" << std::endl << std::endl;
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
		std::cout << "  -t, --stereo          The sound is stereo" << std::endl;
		std::cout << "  -m, --mono            The sound is mono" << std::endl;
		std::cout << std::endl;
		//std::cout << "  --header-size Number  Specify the header size" << std::endl;
		std::cout << "  --input-type Type     Force it to use the decoder for Type (3 or 5)" << std::endl;
		std::cout << "  --sample-rate Rate    Force a specific sampling rate" << std::endl;
		std::cout << std::endl;
	}

	// Check for errors
	if(!ArgParse)
	{
		std::cerr << "Parameters are not valid." << std::endl;
		return 1;
	}

	// Process it based on the action required
	int ReturnValue=1;
	switch(Args.Action)
	{
		case EACT_DECODE:
			ReturnValue=Decode(Args);
		break;
		case EACT_SCAN:
			ReturnValue=Scan(Args);
		break;
	}
	return 0;
}
