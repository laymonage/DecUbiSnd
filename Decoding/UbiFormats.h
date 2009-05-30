// UbiFormats.h : UbiSoft formats
//

#pragma once

#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "Version5Stream.h"
#include "InterleavedStream.h"
#include "OldInterleavedStream.h"
#include "Old6Or4BitStream.h"
#include "OggVorbisStream.h"
#include "RawCompressedStream.h"
#include "RawPcmStream.h"

enum EUbiFormat
{
	EUF_NULL,
	EUF_UBI_V3,
	EUF_UBI_V5,
	EUF_UBI_IV2,
	EUF_UBI_IV8,
	EUF_UBI_6OR4,
	EUF_UBI_RAW,
	EUF_RAW,
	EUF_OGG
};

inline EUbiFormat StringToUbiFormat(const std::string& String)
{
	if(String=="ubi_v3")
	{
		return EUF_UBI_V3;
	}
	else if(String=="ubi_v5")
	{
		return EUF_UBI_V5;
	}
	else if(String=="ubi_iv2")
	{
		return EUF_UBI_IV2;
	}
	else if(String=="ubi_iv8")
	{
		return EUF_UBI_IV8;
	}
	else if(String=="ubi_6or4")
	{
		return EUF_UBI_6OR4;
	}
	else if(String=="ubi_raw")
	{
		return EUF_UBI_RAW;
	}
	else if(String=="raw")
	{
		return EUF_RAW;
	}
	else if(String=="ogg")
	{
		return EUF_OGG;
	}
	return EUF_UBI_RAW;
}
