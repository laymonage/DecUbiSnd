// UbiFormats.h : UbiSoft formats
//

#pragma once

inline EUbiFormat StringToUbiFormat(const std::string& String)
{
	if(String=="ubi_v3")
	{
		return EUF_UBI_V5;
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
