// 6BitAdpcm.h : Decompresses UbiSoft's 6-bit ADPCM
//

#pragma once


struct S6BitAdpcmBlockHeader
{
	unsigned long Signature;
	unsigned long LastIndex1;
	unsigned long Unknown2;
	unsigned long Unknown3;
	unsigned long Unknown4;
	unsigned long Unknown5;
	unsigned long Unknown6;
	unsigned long Unknown7;
	unsigned long LastSample;
	unsigned long Unknown9;
	unsigned long LastIndex2;
	unsigned long Unknown11;
	unsigned long Unknown12;
};

struct S4BitAdpcmBlockHeader
{
	unsigned long Signature;
	unsigned long Unknown1;
	unsigned long Unknown2;
	unsigned long Unknown3;
	unsigned long Unknown4;
	unsigned long Unknown5;
	unsigned long Unknown6;
	unsigned long Unknown7;
	unsigned long Unknown8;
	unsigned long Unknown9;
	unsigned long Unknown10;
	unsigned long Unknown11;
	unsigned long Unknown12;
};

// Get the number of samples in a specified number of bytes 6-bit
unsigned long Get6BitAdpcmSamples(unsigned long Bytes);

// Get the number of samples in a specified number of bytes 4-bit
unsigned long Get4BitAdpcmSamples(unsigned long Bytes);

// Expand a 6-bit block
void Expand6BitAdpcmBlock(void* Source, unsigned long* Dest, unsigned long Count);

// Expand a 4-bit block
void Expand4BitAdpcmBlock(void* Source, unsigned long* Dest, unsigned long Count);

// Decompress a stereo 6-bit block
void DecompressStereo6BitAdpcmBlock(S6BitAdpcmBlockHeader& Left, \
									S6BitAdpcmBlockHeader& Right, \
									unsigned long* Expanded, short* Output, \
									unsigned long SampleCount);

// Decompress a mono 6-bit block
void DecompressMono6BitAdpcmBlock(S6BitAdpcmBlockHeader& Header, \
								  unsigned long* Expanded, short* Output, \
								  unsigned long SampleCount);

// Decompress a stereo 4-bit block
void DecompressStereo4BitAdpcmBlock(S4BitAdpcmBlockHeader& Left, \
									S4BitAdpcmBlockHeader& Right, \
									unsigned long* Expanded, short* Output, \
									unsigned long SampleCount);

// Decompress a mono 4-bit block
void DecompressMono4BitAdpcmBlock(S4BitAdpcmBlockHeader& Header, \
								  unsigned long* Expanded, short* Output, \
								  unsigned long SampleCount);
