#pragma once

#include <cstddef>
#include <stdexcept>

namespace Ubi4BitAdpcm
{
	struct State
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

	static_assert(sizeof(int) == 4, "Ubi 4-bit ADPCM requires 32-bit int");
	static_assert(sizeof(short) == 2, "Ubi 4-bit ADPCM requires 16-bit short");
	static_assert(sizeof(State) == 0x34, "Unexpected Ubi 4-bit ADPCM state layout");
	static_assert(offsetof(State, Coef1) == 0x10, "Unexpected coefficient offset");
	static_assert(offsetof(State, Mod1) == 0x18, "Unexpected modulation offset");
	static_assert(offsetof(State, Hist1) == 0x20, "Unexpected history offset");
	static_assert(offsetof(State, Delta1) == 0x28, "Unexpected delta offset");

	static const int ADPCM4_TABLE1[16] =
	{
		-100000000, 8, 269, 425, 545, 645, 745, 850,
		-1082465976, 1058977874, 1068540887, 1072986849,
		1075167887, 1076761723, 1078439444, 1203982336
	};

	static const int ADPCM4_TABLE2[16] =
	{
		-1536, 2314, 5243, 8192, 14336, 25354, 45445, 143626,
		0, 0, 0, 1, 1, 1, 3, 7
	};

	static const int DELTA_TABLE[66] =
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

	inline int WrapI32(long long Value)
	{
		const unsigned int Bits = static_cast<unsigned int>(Value);
		return Bits < 0x80000000U ? static_cast<int>(Bits)
			: static_cast<int>(static_cast<long long>(Bits) - 0x100000000LL);
	}

	inline short WrapI16(long long Value)
	{
		const unsigned short Bits = static_cast<unsigned short>(Value);
		return Bits < 0x8000U ? static_cast<short>(Bits)
			: static_cast<short>(static_cast<int>(Bits) - 0x10000);
	}

	inline int ArithmeticShiftRight(int Value, unsigned int Shift)
	{
		if(Value >= 0)
		{
			return Value >> Shift;
		}
		return -1 - static_cast<int>((-1LL - Value) >> Shift);
	}

	inline short Clamp16(long long Value)
	{
		if(Value > 32767)
		{
			return 32767;
		}
		if(Value < -32768)
		{
			return -32768;
		}
		return static_cast<short>(Value);
	}

	inline short AbsMax16(short Value, short Maximum)
	{
		if(Value > Maximum)
		{
			return Maximum;
		}
		if(Value < -Maximum)
		{
			return static_cast<short>(-Maximum);
		}
		return Value;
	}

	inline int Sign(int Value)
	{
		return Value < 0 ? -1 : 1;
	}

	inline short DecodeCode(unsigned char Code, State& Channel)
	{
		if(Code > 15)
		{
			throw std::runtime_error("invalid 4-bit ADPCM code");
		}

		const int CodeSigned = static_cast<int>(Code) - 7;
		const unsigned int StepIndex = static_cast<unsigned int>(
			CodeSigned < 0 ? -CodeSigned : CodeSigned);
		const int StepNext = WrapI32(
			static_cast<long long>(ADPCM4_TABLE1[StepIndex]) + Channel.Step1);
		int Step = ArithmeticShiftRight(WrapI32(
			static_cast<long long>(static_cast<unsigned int>(Channel.Step1) & 0xffffU) * 246
				+ ADPCM4_TABLE2[StepIndex]), 8);
		if(Step < 271)
		{
			Step = 271;
		}
		else if(Step > 2560)
		{
			Step = 2560;
		}

		int Delta = 0;
		const unsigned int StepBits = static_cast<unsigned int>(StepNext);
		if((((StepBits & 0xffffff00U) - 1U) & 0x80000000U) == 0)
		{
			const unsigned int DeltaIndex = static_cast<unsigned int>(
				(ArithmeticShiftRight(StepNext, 3) & 0x1f) + (CodeSigned < 0 ? 33 : 0));
			int DeltaShift = (ArithmeticShiftRight(StepNext, 8) & 0xff);
			if(DeltaShift > 31)
			{
				DeltaShift = 31;
			}
			const long long Multiplier = static_cast<long long>(1) << DeltaShift;
			Delta = ArithmeticShiftRight(WrapI32(
				static_cast<long long>(DELTA_TABLE[DeltaIndex]) * Multiplier), 10);
		}

		const short NextValue = WrapI16(ArithmeticShiftRight(WrapI32(
			static_cast<long long>(Channel.Mod1) * Channel.Delta1
			+ static_cast<long long>(Channel.Mod2) * Channel.Delta2
			+ static_cast<long long>(Channel.Mod3) * Channel.Delta3
			+ static_cast<long long>(Channel.Mod4) * Channel.Delta4), 10));
		const int Prediction = ArithmeticShiftRight(WrapI32(
			static_cast<long long>(Channel.Coef1) * Channel.Hist1
			+ static_cast<long long>(Channel.Coef2) * Channel.Hist2), 10);
		const short Sample = WrapI16(WrapI32(
			static_cast<long long>(Delta) + NextValue + Prediction));

		long long Coef1Next = static_cast<long long>(Channel.Coef1) * 255;
		long long Coef2Next = static_cast<long long>(Channel.Coef2) * 254;
		Delta = WrapI16(Delta);
		if(Delta + NextValue != 0)
		{
			const int Sign1 = Sign(Delta + NextValue) * Sign(Channel.Delta1 + Channel.Next1);
			const int Sign2 = Sign(Delta + NextValue) * Sign(Channel.Delta2 + Channel.Next2);
			short CoefDelta = WrapI16(ArithmeticShiftRight(
				Sign1 * 3072 + static_cast<int>(Coef1Next), 6) & ~0x3);
			CoefDelta = Clamp16(static_cast<int>(Clamp16(
				static_cast<int>(CoefDelta) + 30719)) - 30719);
			CoefDelta = Clamp16(static_cast<int>(Clamp16(
				static_cast<int>(CoefDelta) - 30720)) + 30720);
			const int CoefDeltaWide =
				(static_cast<int>(WrapI16(Sign2 * 1024)) - WrapI16(Sign1 * CoefDelta)) * 2;
			Coef1Next += Sign1 * 3072;
			Coef2Next += CoefDeltaWide;
		}

		Channel.Hist2 = Channel.Hist1;
		Channel.Hist1 = Sample;
		Channel.Coef2 = AbsMax16(WrapI16(ArithmeticShiftRight(WrapI32(Coef2Next), 8)), 768);
		Channel.Coef1 = AbsMax16(WrapI16(ArithmeticShiftRight(WrapI32(Coef1Next), 8)),
			static_cast<short>(960 - Channel.Coef2));
		Channel.Next2 = Channel.Next1;
		Channel.Next1 = NextValue;
		Channel.Step1 = Step;
		Channel.Delta5 = Channel.Delta4;
		Channel.Delta4 = Channel.Delta3;
		Channel.Delta3 = Channel.Delta2;
		Channel.Delta2 = Channel.Delta1;
		Channel.Delta1 = static_cast<short>(Delta);
		Channel.Mod4 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(Channel.Mod4) * 255
				+ 2048 * Sign(Channel.Delta1) * Sign(Channel.Delta5)), 8);
		Channel.Mod3 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(Channel.Mod3) * 255
				+ 2048 * Sign(Channel.Delta1) * Sign(Channel.Delta4)), 8);
		Channel.Mod2 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(Channel.Mod2) * 255
				+ 2048 * Sign(Channel.Delta1) * Sign(Channel.Delta3)), 8);
		Channel.Mod1 = ArithmeticShiftRight(Clamp16(
			static_cast<long long>(Channel.Mod1) * 255
				+ 2048 * Sign(Channel.Delta1) * Sign(Channel.Delta2)), 8);

		return Sample;
	}
}
