#include "Pch.h"

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "Sc1AdpcmEncoder.h"

namespace
{
	const size_t GLOBAL_HEADER_SIZE = 48;
	const size_t CHANNEL_HEADER_SIZE = 52;
	const unsigned long EXPECTED_SAMPLE_RATE = 36000;

	const int ADPCM4_TABLE1[8] =
	{
		-100000000, 8, 269, 425, 545, 645, 745, 850
	};
	const int ADPCM4_TABLE2[8] =
	{
		-1536, 2314, 5243, 8192, 14336, 25354, 45445, 143626
	};
	const int DELTA_TABLE[66] =
	{
		1024, 1031, 1053, 1076, 1099, 1123, 1148, 1172,
		1198, 1224, 1251, 1278, 1306, 1334, 1363, 1393,
		1423, 1454, 1485, 1518, 1551, 1584, 1619, 1654,
		1690, 1726, 1764, 1802, 1841, 1881, 1922, 1964,
		2007, -1024, -1031, -1053, -1076, -1099, -1123, -1148,
		-1172, -1198, -1224, -1251, -1278, -1306, -1334, -1363,
		-1393, -1423, -1454, -1485, -1518, -1551, -1584, -1619,
		-1654, -1690, -1726, -1764, -1802, -1841, -1881, -1922,
		-1964, -2007
	};

	struct SChannelState
	{
		int Signature;
		int Step1;
		int Next1;
		int Next2;
		short Coef1;
		short Coef2;
		short Unused1;
		short Unused2;
		short Mod1;
		short Mod2;
		short Mod3;
		short Mod4;
		short Hist1;
		short Hist2;
		short Unused3;
		short Unused4;
		short Delta1;
		short Delta2;
		short Delta3;
		short Delta4;
		short Delta5;
		short Unused5;
	};

	struct SSubframe
	{
		unsigned long CodeCount;
		std::vector<unsigned char> Padding;
	};

	struct SFrame
	{
		SChannelState State;
		std::vector<SSubframe> Subframes;
	};

	struct SSegment
	{
		unsigned long SampleCount;
		unsigned long SubframeCount;
		unsigned long CodesPerSubframeLast;
		unsigned long CodesPerSubframe;
		unsigned long SampleRate;
		unsigned long long Size;
		std::vector<SFrame> Frames;
	};

	unsigned long ReadU32(const unsigned char* Data)
	{
		return static_cast<unsigned long>(Data[0])
			| (static_cast<unsigned long>(Data[1]) << 8)
			| (static_cast<unsigned long>(Data[2]) << 16)
			| (static_cast<unsigned long>(Data[3]) << 24);
	}

	short ReadI16(const unsigned char* Data)
	{
		const unsigned int Value = static_cast<unsigned int>(Data[0])
			| (static_cast<unsigned int>(Data[1]) << 8);
		return Value < 0x8000 ? static_cast<short>(Value)
			: static_cast<short>(static_cast<int>(Value) - 0x10000);
	}

	int ReadI32(const unsigned char* Data)
	{
		const unsigned long Value = ReadU32(Data);
		return Value < 0x80000000UL ? static_cast<int>(Value)
			: static_cast<int>(static_cast<long long>(Value) - 0x100000000LL);
	}

	void AppendU32(std::vector<unsigned char>& Data, unsigned long Value)
	{
		Data.push_back(static_cast<unsigned char>(Value));
		Data.push_back(static_cast<unsigned char>(Value >> 8));
		Data.push_back(static_cast<unsigned char>(Value >> 16));
		Data.push_back(static_cast<unsigned char>(Value >> 24));
	}

	void AppendI16(std::vector<unsigned char>& Data, short Value)
	{
		const unsigned short Bits = static_cast<unsigned short>(Value);
		Data.push_back(static_cast<unsigned char>(Bits));
		Data.push_back(static_cast<unsigned char>(Bits >> 8));
	}

	void AppendI32(std::vector<unsigned char>& Data, int Value)
	{
		AppendU32(Data, static_cast<unsigned long>(Value));
	}

	int WrapI32(long long Value)
	{
		const unsigned long Bits = static_cast<unsigned long>(Value);
		return Bits < 0x80000000UL ? static_cast<int>(Bits)
			: static_cast<int>(static_cast<long long>(Bits) - 0x100000000LL);
	}

	short WrapI16(long long Value)
	{
		const unsigned short Bits = static_cast<unsigned short>(Value);
		return Bits < 0x8000 ? static_cast<short>(Bits)
			: static_cast<short>(static_cast<int>(Bits) - 0x10000);
	}

	int ArithmeticShiftRight(int Value, unsigned int Shift)
	{
		if(Value >= 0)
		{
			return Value >> Shift;
		}
		return -1 - static_cast<int>((-1LL - Value) >> Shift);
	}

	short Clamp16(long long Value)
	{
		return static_cast<short>((std::max)(-32768LL, (std::min)(32767LL, Value)));
	}

	short AbsMax16(short Value, short Maximum)
	{
		return static_cast<short>((std::max)(-static_cast<int>(Maximum),
			(std::min)(static_cast<int>(Maximum), static_cast<int>(Value))));
	}

	int Sign(int Value)
	{
		return Value < 0 ? -1 : 1;
	}

	unsigned long long CheckedAdd(unsigned long long Left, unsigned long long Right)
	{
		if(Right > (std::numeric_limits<unsigned long long>::max)() - Left)
		{
			throw std::runtime_error("segment size overflow");
		}
		return Left + Right;
	}

	void ReadFile(const std::string& Filename, std::vector<unsigned char>& Data)
	{
		std::ifstream Input(Filename.c_str(), std::ios::in | std::ios::binary);
		if(!Input)
		{
			throw std::runtime_error("unable to open input file '" + Filename + "'");
		}
		Input.seekg(0, std::ios::end);
		const std::streamoff Length = Input.tellg();
		if(Length < 0 || static_cast<unsigned long long>(Length) > Data.max_size())
		{
			throw std::runtime_error("input file is too large or could not be sized: '" + Filename + "'");
		}
		Input.seekg(0, std::ios::beg);
		Data.resize(static_cast<size_t>(Length));
		if(Length && !Input.read(reinterpret_cast<char*>(&Data[0]), static_cast<std::streamsize>(Length)))
		{
			throw std::runtime_error("unable to read input file '" + Filename + "'");
		}
	}

	void ReadState(const unsigned char* Data, SChannelState& State)
	{
		State.Signature = ReadI32(Data);
		State.Step1 = ReadI32(Data + 4);
		State.Next1 = ReadI32(Data + 8);
		State.Next2 = ReadI32(Data + 12);
		short* Fields[] =
		{
			&State.Coef1, &State.Coef2, &State.Unused1, &State.Unused2,
			&State.Mod1, &State.Mod2, &State.Mod3, &State.Mod4,
			&State.Hist1, &State.Hist2, &State.Unused3, &State.Unused4,
			&State.Delta1, &State.Delta2, &State.Delta3, &State.Delta4,
			&State.Delta5, &State.Unused5
		};
		for(size_t Index = 0; Index < sizeof(Fields) / sizeof(Fields[0]); ++Index)
		{
			*Fields[Index] = ReadI16(Data + 16 + Index * 2);
		}
	}

	void AppendState(std::vector<unsigned char>& Data, const SChannelState& State)
	{
		AppendI32(Data, State.Signature);
		AppendI32(Data, State.Step1);
		AppendI32(Data, State.Next1);
		AppendI32(Data, State.Next2);
		const short Fields[] =
		{
			State.Coef1, State.Coef2, State.Unused1, State.Unused2,
			State.Mod1, State.Mod2, State.Mod3, State.Mod4,
			State.Hist1, State.Hist2, State.Unused3, State.Unused4,
			State.Delta1, State.Delta2, State.Delta3, State.Delta4,
			State.Delta5, State.Unused5
		};
		for(size_t Index = 0; Index < sizeof(Fields) / sizeof(Fields[0]); ++Index)
		{
			AppendI16(Data, Fields[Index]);
		}
	}

	std::vector<unsigned long> GetSubframeCounts(const SSegment& Segment)
	{
		if(!Segment.SubframeCount || Segment.SubframeCount > Segment.SampleCount
			|| !Segment.CodesPerSubframe || !Segment.CodesPerSubframeLast)
		{
			throw std::runtime_error("invalid ADPCM subframe counts in segment header");
		}
		std::vector<unsigned long> Counts;
		Counts.reserve(Segment.SubframeCount);
		while(Counts.size() < Segment.SubframeCount)
		{
			const size_t Remaining = Segment.SubframeCount - Counts.size();
			if(Remaining == 1)
			{
				Counts.push_back(Segment.CodesPerSubframeLast);
			}
			else if(Remaining == 2)
			{
				Counts.push_back(Segment.CodesPerSubframe);
				Counts.push_back(Segment.CodesPerSubframeLast);
			}
			else
			{
				Counts.push_back(Segment.CodesPerSubframe);
				Counts.push_back(Segment.CodesPerSubframe);
			}
		}
		return Counts;
	}

	SSegment ParseSegment(const unsigned char* Data, size_t Available)
	{
		if(Available < GLOBAL_HEADER_SIZE)
		{
			throw std::runtime_error("target offset is beyond a complete Ubisoft audio header");
		}
		if(ReadU32(Data) != 8 || ReadU32(Data + 36) != 4 || ReadU32(Data + 44) != 1)
		{
			throw std::runtime_error("only mono Ubisoft 4-bit segments are supported");
		}
		if(ReadU32(Data + 20) != 2)
		{
			throw std::runtime_error("unsupported subframes-per-frame value");
		}

		SSegment Segment;
		Segment.SampleCount = ReadU32(Data + 4);
		Segment.SubframeCount = ReadU32(Data + 8);
		Segment.CodesPerSubframeLast = ReadU32(Data + 12);
		Segment.CodesPerSubframe = ReadU32(Data + 16);
		Segment.SampleRate = ReadU32(Data + 24);
		if(!Segment.SampleCount || Segment.SampleRate != EXPECTED_SAMPLE_RATE)
		{
			throw std::runtime_error("segment must declare samples at 36000 Hz");
		}

		const std::vector<unsigned long> Counts = GetSubframeCounts(Segment);
		unsigned long long TotalSamples = 0;
		for(size_t Index = 0; Index < Counts.size(); ++Index)
		{
			TotalSamples = CheckedAdd(TotalSamples, Counts[Index]);
		}
		if(TotalSamples != Segment.SampleCount)
		{
			throw std::runtime_error("segment header sample count does not match its subframes");
		}

		unsigned long long Cursor = GLOBAL_HEADER_SIZE;
		size_t CountIndex = 0;
		while(CountIndex < Counts.size())
		{
			if(CheckedAdd(Cursor, CHANNEL_HEADER_SIZE) > Available)
			{
				throw std::runtime_error("truncated ADPCM channel state in target segment");
			}
			SFrame Frame;
			ReadState(Data + static_cast<size_t>(Cursor), Frame.State);
			if(Frame.State.Signature != 2)
			{
				throw std::runtime_error("unsupported ADPCM channel-state signature");
			}
			Cursor += CHANNEL_HEADER_SIZE;
			const size_t FrameSubframes = (std::min)(static_cast<size_t>(2), Counts.size() - CountIndex);
			for(size_t SubframeIndex = 0; SubframeIndex < FrameSubframes; ++SubframeIndex)
			{
				const unsigned long CodeCount = Counts[CountIndex + SubframeIndex];
				const unsigned long long DataSize = (static_cast<unsigned long long>(CodeCount) + 1) / 2;
				const bool FinalPartial = CountIndex + SubframeIndex == Counts.size() - 1
					&& CodeCount % 8 != 0;
				const unsigned long long PaddingSize = FinalPartial ? 0 : 1;
				const unsigned long long End = CheckedAdd(Cursor, CheckedAdd(DataSize, PaddingSize));
				if(End > Available)
				{
					throw std::runtime_error("truncated ADPCM subframe in target segment");
				}
				SSubframe Subframe;
				Subframe.CodeCount = CodeCount;
				if(PaddingSize)
				{
					Subframe.Padding.push_back(Data[static_cast<size_t>(Cursor + DataSize)]);
				}
				Frame.Subframes.push_back(Subframe);
				Cursor = End;
			}
			Segment.Frames.push_back(Frame);
			CountIndex += FrameSubframes;
		}
		const unsigned long long AlignedSize = CheckedAdd(Cursor, (4 - (Cursor & 3)) & 3);
		if(AlignedSize > Available)
		{
			throw std::runtime_error("target segment is missing trailing alignment padding");
		}
		Segment.Size = AlignedSize;
		return Segment;
	}

	void AppendPcmCodes(std::vector<unsigned char>& Output, const std::vector<unsigned char>& Codes)
	{
		size_t Index = 0;
		const size_t CompleteCount = Codes.size() - Codes.size() % 8;
		for(; Index < CompleteCount; Index += 8)
		{
			unsigned long Word = 0;
			for(size_t CodeIndex = 0; CodeIndex < 8; ++CodeIndex)
			{
				if(Codes[Index + CodeIndex] > 14)
				{
					throw std::runtime_error("unsupported 4-bit ADPCM code");
				}
				Word = (Word << 4) | Codes[Index + CodeIndex];
			}
			AppendU32(Output, Word);
		}
		const size_t Remaining = Codes.size() - CompleteCount;
		if(Remaining)
		{
			unsigned long Word = 0;
			for(size_t CodeIndex = 0; CodeIndex < Remaining; ++CodeIndex)
			{
				if(Codes[CompleteCount + CodeIndex] > 14)
				{
					throw std::runtime_error("unsupported 4-bit ADPCM code");
				}
				Word = (Word << 4) | Codes[CompleteCount + CodeIndex];
			}
			Word <<= (8 - Remaining) * 4;
			const size_t ByteCount = (Remaining + 1) / 2;
			for(size_t ByteIndex = 0; ByteIndex < ByteCount; ++ByteIndex)
			{
					Output.push_back(static_cast<unsigned char>(
						Word >> ((4 - ByteCount + ByteIndex) * 8)));
				}
		}
	}

	short ExpandCode4Bit(unsigned char Code, SChannelState& State)
	{
		const int CodeSigned = static_cast<int>(Code) - 7;
		const unsigned int StepIndex = static_cast<unsigned int>(CodeSigned < 0 ? -CodeSigned : CodeSigned);
		if(StepIndex > 7)
		{
			throw std::runtime_error("4-bit code is outside the supported range 0..14");
		}
		const int StepNext = WrapI32(static_cast<long long>(ADPCM4_TABLE1[StepIndex]) + State.Step1);
		int Step = ArithmeticShiftRight(WrapI32(
			static_cast<long long>(static_cast<unsigned int>(State.Step1) & 0xffff) * 246
				+ ADPCM4_TABLE2[StepIndex]), 8);
		Step = (std::max)(271, (std::min)(2560, Step));

		int Delta = 0;
		const unsigned long StepBits = static_cast<unsigned long>(StepNext);
		if((((StepBits & 0xffffff00UL) - 1) & 0x80000000UL) == 0)
		{
			const unsigned int DeltaIndex = static_cast<unsigned int>(
				(ArithmeticShiftRight(StepNext, 3) & 0x1f) + (CodeSigned < 0 ? 33 : 0));
			const unsigned int DeltaShift = static_cast<unsigned int>(
				(std::max)(0, (std::min)(31, static_cast<int>(
					(static_cast<unsigned long>(ArithmeticShiftRight(StepNext, 8)) & 0xff)))));
			const long long Multiplier = static_cast<long long>(1) << DeltaShift;
			Delta = ArithmeticShiftRight(WrapI32(
				static_cast<long long>(DELTA_TABLE[DeltaIndex]) * Multiplier), 10);
		}

		const short NextValue = WrapI16(ArithmeticShiftRight(WrapI32(
			static_cast<long long>(State.Mod1) * State.Delta1
			+ static_cast<long long>(State.Mod2) * State.Delta2
			+ static_cast<long long>(State.Mod3) * State.Delta3
			+ static_cast<long long>(State.Mod4) * State.Delta4), 10));
		const int Prediction = ArithmeticShiftRight(WrapI32(
			static_cast<long long>(State.Coef1) * State.Hist1
			+ static_cast<long long>(State.Coef2) * State.Hist2), 10);
		const short Sample = WrapI16(WrapI32(static_cast<long long>(Delta) + NextValue + Prediction));

		long long Coef1Next = static_cast<long long>(State.Coef1) * 255;
		long long Coef2Next = static_cast<long long>(State.Coef2) * 254;
		Delta = WrapI16(Delta);
		if(Delta + NextValue)
		{
			const int Sign1 = Sign(Delta + NextValue) * Sign(State.Delta1 + State.Next1);
			const int Sign2 = Sign(Delta + NextValue) * Sign(State.Delta2 + State.Next2);
			short CoefDelta = WrapI16(ArithmeticShiftRight(Sign1 * 3072 + static_cast<int>(Coef1Next), 6) & ~0x3);
			CoefDelta = Clamp16(static_cast<int>(Clamp16(static_cast<int>(CoefDelta) + 30719)) - 30719);
			CoefDelta = Clamp16(static_cast<int>(Clamp16(static_cast<int>(CoefDelta) - 30720)) + 30720);
			const int CoefDeltaWide =
				(static_cast<int>(WrapI16(Sign2 * 1024)) - WrapI16(Sign1 * CoefDelta)) * 2;
			Coef1Next += Sign1 * 3072;
			Coef2Next += CoefDeltaWide;
		}

		State.Hist2 = State.Hist1;
		State.Hist1 = Sample;
		State.Coef2 = AbsMax16(WrapI16(ArithmeticShiftRight(WrapI32(Coef2Next), 8)), 768);
		State.Coef1 = AbsMax16(WrapI16(ArithmeticShiftRight(WrapI32(Coef1Next), 8)),
			static_cast<short>(960 - State.Coef2));
		State.Next2 = State.Next1;
		State.Next1 = NextValue;
		State.Step1 = Step;
		State.Delta5 = State.Delta4;
		State.Delta4 = State.Delta3;
		State.Delta3 = State.Delta2;
		State.Delta2 = State.Delta1;
		State.Delta1 = static_cast<short>(Delta);
		State.Mod4 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(State.Mod4) * 255 + 2048 * Sign(State.Delta1) * Sign(State.Delta5)), 8);
		State.Mod3 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(State.Mod3) * 255 + 2048 * Sign(State.Delta1) * Sign(State.Delta4)), 8);
		State.Mod2 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(State.Mod2) * 255 + 2048 * Sign(State.Delta1) * Sign(State.Delta3)), 8);
		State.Mod1 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(State.Mod1) * 255 + 2048 * Sign(State.Delta1) * Sign(State.Delta2)), 8);
		return Sample;
	}

	std::vector<unsigned char> PackCodes(const std::vector<unsigned char>& Codes)
	{
		std::vector<unsigned char> Packed;
		AppendPcmCodes(Packed, Codes);
		return Packed;
	}

	void ReadWave(const std::string& Filename, unsigned long ExpectedRate,
		unsigned long ExpectedSamples, std::vector<short>& Samples)
	{
		std::vector<unsigned char> Wave;
		ReadFile(Filename, Wave);
		if(Wave.size() < 12 || memcmp(&Wave[0], "RIFF", 4) != 0
			|| memcmp(&Wave[8], "WAVE", 4) != 0)
		{
			throw std::runtime_error("replacement WAV is not a RIFF/WAVE file");
		}
		bool HaveFormat = false;
		bool HaveData = false;
		unsigned long SampleRate = 0;
		unsigned long Channels = 0;
		unsigned long BitsPerSample = 0;
		unsigned long BlockAlign = 0;
		size_t DataOffset = 0;
		unsigned long DataSize = 0;
		unsigned long long Cursor = 12;
		while(Cursor + 8 <= Wave.size())
		{
			const unsigned char* Chunk = &Wave[static_cast<size_t>(Cursor)];
			const unsigned long ChunkSize = ReadU32(Chunk + 4);
			const unsigned long long ChunkEnd = CheckedAdd(Cursor, CheckedAdd(8, ChunkSize));
			if(ChunkEnd > Wave.size())
			{
				throw std::runtime_error("replacement WAV contains a truncated chunk");
			}
			if(memcmp(Chunk, "fmt ", 4) == 0)
			{
				if(ChunkSize < 16)
				{
					throw std::runtime_error("replacement WAV format chunk is too short");
				}
				const unsigned int Format = static_cast<unsigned int>(Chunk[8])
					| (static_cast<unsigned int>(Chunk[9]) << 8);
				if(Format != 1)
				{
					throw std::runtime_error("replacement WAV must use uncompressed PCM");
				}
				Channels = static_cast<unsigned long>(Chunk[10])
					| (static_cast<unsigned long>(Chunk[11]) << 8);
				SampleRate = ReadU32(Chunk + 12);
				BlockAlign = static_cast<unsigned long>(Chunk[20])
					| (static_cast<unsigned long>(Chunk[21]) << 8);
				BitsPerSample = static_cast<unsigned long>(Chunk[22])
					| (static_cast<unsigned long>(Chunk[23]) << 8);
				HaveFormat = true;
			}
			else if(memcmp(Chunk, "data", 4) == 0 && !HaveData)
			{
				DataOffset = static_cast<size_t>(Cursor + 8);
				DataSize = ChunkSize;
				HaveData = true;
			}
			Cursor = CheckedAdd(ChunkEnd, ChunkSize & 1);
		}
		if(!HaveFormat || !HaveData)
		{
			throw std::runtime_error("replacement WAV is missing its format or data chunk");
		}
		if(Channels != 1 || BitsPerSample != 16 || BlockAlign != 2)
		{
			throw std::runtime_error("replacement WAV must be mono, signed 16-bit PCM");
		}
		if(SampleRate != ExpectedRate)
		{
			throw std::runtime_error("replacement WAV sample rate is "
				+ std::to_string(SampleRate) + " Hz; expected "
				+ std::to_string(ExpectedRate) + " Hz");
		}
		if(DataSize % 2 || DataSize / 2 != ExpectedSamples)
		{
			throw std::runtime_error("replacement WAV contains "
				+ std::to_string(DataSize / 2) + " samples; expected "
				+ std::to_string(ExpectedSamples));
		}
		Samples.resize(ExpectedSamples);
		for(size_t Index = 0; Index < Samples.size(); ++Index)
		{
			Samples[Index] = ReadI16(&Wave[DataOffset + Index * 2]);
		}
	}

	void AppendEncodedSubframe(std::vector<unsigned char>& Output, const std::vector<short>& Samples,
		size_t& SampleOffset, const SSubframe& Subframe, SChannelState& State)
	{
		std::vector<unsigned char> Codes;
		Codes.reserve(Subframe.CodeCount);
		for(unsigned long Index = 0; Index < Subframe.CodeCount; ++Index)
		{
			unsigned char BestCode = 0;
			unsigned long long BestError = (std::numeric_limits<unsigned long long>::max)();
			SChannelState BestState = State;
			for(unsigned char Code = 0; Code < 15; ++Code)
			{
				SChannelState Candidate = State;
				const short Reconstructed = ExpandCode4Bit(Code, Candidate);
				const long long Difference = static_cast<long long>(Samples[SampleOffset]) - Reconstructed;
				const unsigned long long Error = static_cast<unsigned long long>(Difference * Difference);
				if(Error < BestError)
				{
					BestCode = Code;
					BestError = Error;
					BestState = Candidate;
				}
			}
			Codes.push_back(BestCode);
			State = BestState;
			++SampleOffset;
		}
		std::vector<unsigned char> Packed = PackCodes(Codes);
		const unsigned long long DataSize = (static_cast<unsigned long long>(Subframe.CodeCount) + 1) / 2;
		if(Packed.size() != DataSize)
		{
			throw std::runtime_error("encoded ADPCM subframe does not fit the template layout");
		}
		Output.insert(Output.end(), Packed.begin(), Packed.end());
		Output.insert(Output.end(), Subframe.Padding.begin(), Subframe.Padding.end());
	}

	std::vector<unsigned char> EncodeSegment(const unsigned char* Template, const SSegment& Segment,
		const std::vector<short>& Samples)
	{
		std::vector<unsigned char> Output;
		if(Segment.Size > Output.max_size())
		{
			throw std::runtime_error("target segment is too large to encode");
		}
		Output.reserve(static_cast<size_t>(Segment.Size));
		Output.insert(Output.end(), Template, Template + GLOBAL_HEADER_SIZE);
		SChannelState State = Segment.Frames[0].State;
		size_t SampleOffset = 0;
		for(size_t FrameIndex = 0; FrameIndex < Segment.Frames.size(); ++FrameIndex)
		{
			AppendState(Output, State);
			const SFrame& Frame = Segment.Frames[FrameIndex];
			for(size_t SubframeIndex = 0; SubframeIndex < Frame.Subframes.size(); ++SubframeIndex)
			{
				AppendEncodedSubframe(Output, Samples, SampleOffset, Frame.Subframes[SubframeIndex], State);
			}
		}
		if(SampleOffset != Samples.size())
		{
			throw std::runtime_error("encoded sample count does not match the segment header");
		}
		const size_t ParsedSize = static_cast<size_t>(Segment.Size);
		const size_t Used = Output.size();
		if(Used > ParsedSize)
		{
			throw std::runtime_error("encoded segment exceeds the template size");
		}
		Output.insert(Output.end(), Template + Used, Template + ParsedSize);
		if(Output.size() != ParsedSize)
		{
			throw std::runtime_error("encoded segment size changed");
		}
		return Output;
	}

	std::string GetFullPath(const std::string& Filename)
	{
		const DWORD Required = GetFullPathNameA(Filename.c_str(), 0, NULL, NULL);
		if(!Required)
		{
			throw std::runtime_error("unable to resolve path '" + Filename + "'");
		}
		std::vector<char> Buffer(Required + 1);
		const DWORD Written = GetFullPathNameA(Filename.c_str(), static_cast<DWORD>(Buffer.size()),
			&Buffer[0], NULL);
		if(!Written || Written >= Buffer.size())
		{
			throw std::runtime_error("unable to resolve path '" + Filename + "'");
		}
		return std::string(&Buffer[0], Written);
	}

	bool IsSameFile(HANDLE Left, HANDLE Right)
	{
		BY_HANDLE_FILE_INFORMATION LeftInfo;
		BY_HANDLE_FILE_INFORMATION RightInfo;
		if(!GetFileInformationByHandle(Left, &LeftInfo)
			|| !GetFileInformationByHandle(Right, &RightInfo))
		{
			return false;
		}
		return LeftInfo.dwVolumeSerialNumber == RightInfo.dwVolumeSerialNumber
			&& LeftInfo.nFileIndexHigh == RightInfo.nFileIndexHigh
			&& LeftInfo.nFileIndexLow == RightInfo.nFileIndexLow;
	}

	bool OutputAliasesFile(const std::string& InputFilename, const std::string& OutputFilename)
	{
		if(_stricmp(GetFullPath(InputFilename).c_str(), GetFullPath(OutputFilename).c_str()) == 0)
		{
			return true;
		}
		HANDLE Input = CreateFileA(InputFilename.c_str(), FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
		if(Input == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		HANDLE Output = CreateFileA(OutputFilename.c_str(), FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
		const bool Same = Output != INVALID_HANDLE_VALUE && IsSameFile(Input, Output);
		if(Output != INVALID_HANDLE_VALUE)
		{
			CloseHandle(Output);
		}
		CloseHandle(Input);
		return Same;
	}

	std::string GetWindowsError(const char* Action, const std::string& Filename)
	{
		return std::string(Action) + " '" + Filename + "' (Windows error "
			+ std::to_string(GetLastError()) + ")";
	}

	void WriteFileAtomically(const std::string& Filename, const std::vector<unsigned char>& Data)
	{
		std::string Temporary;
		HANDLE File = INVALID_HANDLE_VALUE;
		for(unsigned int Attempt = 0; Attempt < 32 && File == INVALID_HANDLE_VALUE; ++Attempt)
		{
			Temporary = Filename + ".tmp." + std::to_string(GetCurrentProcessId())
				+ "." + std::to_string(GetTickCount() + Attempt);
			File = CreateFileA(Temporary.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL, NULL);
			const DWORD Error = GetLastError();
			if(File == INVALID_HANDLE_VALUE && Error != ERROR_FILE_EXISTS
				&& Error != ERROR_ALREADY_EXISTS)
			{
				throw std::runtime_error(GetWindowsError("unable to create temporary output", Temporary));
			}
		}
		if(File == INVALID_HANDLE_VALUE)
		{
			throw std::runtime_error("unable to create a unique temporary output file");
		}

		bool Success = true;
		size_t Offset = 0;
		while(Offset < Data.size())
		{
			const DWORD Remaining = static_cast<DWORD>((std::min)(
				Data.size() - Offset, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
			DWORD Written = 0;
			if(!WriteFile(File, &Data[Offset], Remaining, &Written, NULL) || Written != Remaining)
			{
				Success = false;
				break;
			}
			Offset += Written;
		}
		if(Success && !FlushFileBuffers(File))
		{
			Success = false;
		}
		const DWORD WriteError = GetLastError();
		CloseHandle(File);
		if(!Success)
		{
			DeleteFileA(Temporary.c_str());
			throw std::runtime_error("unable to write complete output bank (Windows error "
				+ std::to_string(WriteError) + ")");
		}
		if(!MoveFileExA(Temporary.c_str(), Filename.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			const std::string Error = GetWindowsError("unable to replace output bank", Filename);
			DeleteFileA(Temporary.c_str());
			throw std::runtime_error(Error);
		}
	}
}

SSc1SegmentInfo InspectSc1AdpcmSegment(const std::string& BankFilename, unsigned long long Offset)
{
	if(BankFilename.empty())
	{
		throw std::runtime_error("source bank filename is required");
	}
	std::vector<unsigned char> Bank;
	ReadFile(BankFilename, Bank);
	if(Offset > Bank.size() || Bank.size() - static_cast<size_t>(Offset) < GLOBAL_HEADER_SIZE)
	{
		throw std::runtime_error("target offset is beyond the end of the source bank");
	}
	const SSegment Segment = ParseSegment(&Bank[static_cast<size_t>(Offset)],
		Bank.size() - static_cast<size_t>(Offset));
	SSc1SegmentInfo Info;
	Info.SampleRate = Segment.SampleRate;
	Info.SampleCount = Segment.SampleCount;
	Info.SegmentSize = Segment.Size;
	return Info;
}

SSc1SegmentInfo ReplaceSc1AdpcmSegment(const std::string& BankFilename,
	const std::string& WaveFilename, const std::string& OutputFilename, unsigned long long Offset)
{
	if(BankFilename.empty() || WaveFilename.empty() || OutputFilename.empty())
	{
		throw std::runtime_error("bank, replacement WAV, and output filenames are required");
	}
	if(OutputAliasesFile(WaveFilename, OutputFilename))
	{
		throw std::runtime_error("output must be a different file from the replacement WAV");
	}

	std::vector<unsigned char> Bank;
	ReadFile(BankFilename, Bank);
	if(Offset > Bank.size() || Bank.size() - static_cast<size_t>(Offset) < GLOBAL_HEADER_SIZE)
	{
		throw std::runtime_error("target offset is beyond the end of the source bank");
	}
	const size_t BankOffset = static_cast<size_t>(Offset);
	const SSegment Segment = ParseSegment(&Bank[BankOffset], Bank.size() - BankOffset);
	std::vector<short> Samples;
	ReadWave(WaveFilename, Segment.SampleRate, Segment.SampleCount, Samples);
	const std::vector<unsigned char> Replacement =
		EncodeSegment(&Bank[BankOffset], Segment, Samples);
	if(Replacement.size() != Segment.Size)
	{
		throw std::runtime_error("replacement segment size changed; refusing to rewrite the bank");
	}
	std::vector<unsigned char> Output;
	Output.reserve(Bank.size());
	Output.insert(Output.end(), Bank.begin(), Bank.begin() + BankOffset);
	Output.insert(Output.end(), Replacement.begin(), Replacement.end());
	Output.insert(Output.end(), Bank.begin() + BankOffset + Replacement.size(), Bank.end());
	if(Output.size() != Bank.size())
	{
		throw std::runtime_error("replacement changed the source bank size");
	}
	WriteFileAtomically(OutputFilename, Output);

	SSc1SegmentInfo Result;
	Result.SampleRate = Segment.SampleRate;
	Result.SampleCount = Segment.SampleCount;
	Result.SegmentSize = Segment.Size;
	return Result;
}
