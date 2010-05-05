/*
 Output.cpp : Output a segment stream
*/

#include "Pch.h"

#include <wx/progdlg.h>
#include <wx/filename.h>

#include "Functionality/Output.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"
#include "Decoding/WaveWriter.h"
#include "Decoding/AudioStream.h"
#include "Decoding/FileDataStream.h"
#include "Decoding/Version5Stream.h"
#include "Decoding/InterleavedStream.h"
#include "Decoding/Interleaved9Stream.h"
#include "Decoding/OldInterleavedStream.h"
#include "Decoding/OggVorbisStream.h"
#include "Decoding/RawCompressedStream.h"
#include "Decoding/RawPcmStream.h"

bool NDecFunc::OutputConcatenated(const std::string Filename, const std::vector<CSegment*>& Segments)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Output Concatenated Stream..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

	// Open the file
	Progress.Pulse(_("Opening the file..."));
	std::ofstream Output;
	Output.open(Filename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		return false;
	}

	// Prepare the wave header
	Progress.Pulse(_("Writing the wave header..."));
	PrepareWaveHeader(Output);

	// Create the segment stream
	Progress.Pulse(_("Creating the segment stream..."));
	CSegmentStream Stream;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		Stream.Add(*Iter);
	}

	// Create the buffers
	Progress.Pulse(_("Decoding audio..."));
	unsigned long NumberDecodedSamples=0;
	unsigned long OutputBufferLength=65536;
	short* OutputBuffer=new short[OutputBufferLength];

	// Do the loop
	while(true)
	{
		// Decode some
		unsigned long LocalSamples=OutputBufferLength;
		if(!Stream.Decode(OutputBuffer, LocalSamples))
		{
			if(LocalSamples)
			{
				NumberDecodedSamples+=LocalSamples;
				Output.write((char*)OutputBuffer, LocalSamples*2);
			}
			wxMessageBox(_("Had some problems decoding to an output file"));
			break;
		}

		// Check if any was decoded
		if(LocalSamples==0)
		{
			break;
		}

		// Write it to the output stream
		NumberDecodedSamples+=LocalSamples;
		Output.write((char*)OutputBuffer, LocalSamples*2);

		Progress.Update(Stream.GetProgess(100), _("Decoding audio..."));
	}

	// Clean up
	delete [] OutputBuffer;

	// Update the wave header
	Progress.Update(100, _("Finishing up..."));
	if(Stream.GetCurrentSegment())
	{
		CSegment* Segment=Stream.GetCurrentSegment();
		Output.seekp(0);
		WriteWaveHeader(Output, Segment->GetSampleRate(), 16, Segment->GetChannels(),\
			NumberDecodedSamples);
	}

	// Close the file
	Output.close();
	return true;
}

bool NDecFunc::OutputSeparate(const std::string DirName, const std::vector<CSegment*>& Segments)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Output Separate Streams..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

	// Process each segment
	unsigned long i=0;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		// Get the segment
		const CSegment& Segment=*(*Iter);

		// Update the progress dialog
		Progress.Update(i*100/Segments.size(), wxString::Format(_("Decoding audio %lu/%lu..."), i+1, Segments.size()));
		
		// Get the appropriate filename
		std::string Filename;
		wxFileName InputFilename(Segment.GetFilename());
		wxFileName OutputFilename;
		OutputFilename.SetPath(DirName);
		OutputFilename.SetName(InputFilename.GetName()+wxString::Format(wxT("_%lu"), i));
		OutputFilename.SetExt(wxT("wav"));
		Filename=OutputFilename.GetFullPath();

		// Open the file
		std::ofstream Output;
		Output.open(Filename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		if(!Output.is_open())
		{
			return false;
		}

		// Prepare the wave header
		PrepareWaveHeader(Output);

		// Create the segment stream
		CSegmentStream Stream;
		Stream.Add(&Segment);

		// Create the buffers
		unsigned long NumberDecodedSamples=0;
		unsigned long OutputBufferLength=65536;
		short* OutputBuffer=new short[OutputBufferLength];

		// Do the loop
		while(true)
		{
			// Decode some
			unsigned long LocalSamples=OutputBufferLength;
			if(!Stream.Decode(OutputBuffer, LocalSamples))
			{
				if(LocalSamples)
				{
					NumberDecodedSamples+=LocalSamples;
					Output.write((char*)OutputBuffer, LocalSamples*2);
				}
				wxMessageBox(_("Had some problems decoding to an output file"));
				break;
			}

			// Check if any was decoded
			if(LocalSamples==0)
			{
				break;
			}

			// Write it to the output stream
			NumberDecodedSamples+=LocalSamples;
			Output.write((char*)OutputBuffer, LocalSamples*2);

			Progress.Update(i*100/Segments.size()+Stream.GetProgess(100/Segments.size()), wxString::Format(_("Decoding audio %lu/%lu..."), i+1, Segments.size()));
		}

		// Write the wave header
		Output.seekp(0);
		WriteWaveHeader(Output, Segment.GetSampleRate(), 16, Segment.GetChannels(),\
			NumberDecodedSamples);

		// Clean up
		delete [] OutputBuffer;
		Output.close();

		// Update
		i++;
	}

	// Finish up
	Progress.Update(100, _("Finishing up..."));
	return true;
}

bool NDecFunc::OutputLayerExtract(const std::string DirName, const std::vector<CSegment*>& Segments, const wxString& OutputFilename)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Output Layer Extract..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

	// Process each segment
	unsigned long i=0;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		// Get the segment
		const CSegment& Segment=*(*Iter);

		// Update the progress dialog
		Progress.Update(i*100/Segments.size(), wxString::Format(_("Extracting audio %lu/%lu..."), i+1, Segments.size()));
		
		// Get the appropriate filename
		std::string Filename;
		wxFileName InputFilename(Segment.GetFilename());
		wxFileName OutputFilename;
		OutputFilename.SetPath(DirName);
		OutputFilename.SetName(InputFilename.GetName()+wxString::Format(wxT("_%lu"), i));
		OutputFilename.SetExt(wxT("layer"));
		Filename=OutputFilename.GetFullPath();

		// Open the input stream
		std::ifstream Input;
		Input.open(Segment.GetFilename().c_str(), std::ios_base::in | std::ios_base::binary);
		if(!Input.is_open())
		{
			return false;
		}

		// Open the output file
		std::ofstream Output;
		Output.open(Filename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		if(!Output.is_open())
		{
			return false;
		}

		// Determine the type
		EUbiFormat Type;
		Input.seekg(Segment.GetOffset());
		Type=Segment.GetType();

		// Set up an input stream
		CFileDataStream FileStream(&Input, Segment.GetOffset(), Segment.GetSize());

		// Get the layer
		unsigned long Layer;
		if(Type!=EUF_UBI_IV2 && Type!=EUF_UBI_IV8 && Type!=EUF_UBI_IV9)
		{
			// Do a generic copy
			while(!FileStream.IsEnd())
			{
				unsigned long Read=65536;
				char* Buffer;
				Buffer=(char*)FileStream.Read(Read);
				Output.write(Buffer, Read);
			}
		}
		else
		{
			// Set the layer
			Layer=Segment.GetLayers()[0];

			// Do the layer extract
			try
			{
				if(Type==EUF_UBI_IV8)
				{
					if(!CInterleavedStream::LayerExtract(&FileStream, Layer, Output))
					{
						return false;
					}
				}
				else if(Type==EUF_UBI_IV9)
				{
					if(!CInterleaved9Stream::LayerExtract(&FileStream, Layer, Output))
					{
						return false;
					}
				}
				else if(Type==EUF_UBI_IV2)
				{
					if(!COldInterleavedStream::LayerExtract(&FileStream, Layer, Output))
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			catch(...)
			{
				return false;
			}
		}

		// Clean up
		Input.close();
		Output.close();

		// Update
		i++;
	}

	// Finish up
	Progress.Update(100, _("Finishing up..."));
	return true;
}
