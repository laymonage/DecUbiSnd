#include "Pch.h"

#define NOMINMAX
#include <windows.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "AudioConverter.h"

namespace
{
	std::wstring QuoteArgument(const std::wstring& Argument)
	{
		std::wstring Quoted(1, L'"');
		size_t Backslashes = 0;
		for(std::wstring::const_iterator Iter = Argument.begin(); Iter != Argument.end(); ++Iter)
		{
			if(*Iter == L'\\')
			{
				++Backslashes;
			}
			else if(*Iter == L'"')
			{
				Quoted.append(Backslashes * 2 + 1, L'\\');
				Quoted.push_back(*Iter);
				Backslashes = 0;
			}
			else
			{
				Quoted.append(Backslashes, L'\\');
				Quoted.push_back(*Iter);
				Backslashes = 0;
			}
		}
		Quoted.append(Backslashes * 2, L'\\');
		Quoted.push_back(L'"');
		return Quoted;
	}

	std::wstring MakeCommandLine(const std::wstring& Executable,
		const std::vector<std::wstring>& Arguments)
	{
		std::wstring Command = QuoteArgument(Executable);
		for(std::vector<std::wstring>::const_iterator Iter = Arguments.begin();
			Iter != Arguments.end(); ++Iter)
		{
			Command.push_back(L' ');
			Command.append(QuoteArgument(*Iter));
		}
		if(Command.size() >= 32767)
		{
			throw std::runtime_error("FFmpeg command line is too long");
		}
		return Command;
	}

	std::string FormatWindowsError(const char* Action)
	{
		return std::string(Action) + " (Windows error "
			+ std::to_string(GetLastError()) + ")";
	}

	DWORD RunProcess(const std::wstring& Executable,
		const std::vector<std::wstring>& Arguments, std::string* StandardOutput)
	{
		SECURITY_ATTRIBUTES Security;
		ZeroMemory(&Security, sizeof(Security));
		Security.nLength = sizeof(Security);
		Security.bInheritHandle = TRUE;

		HANDLE NullDevice = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, &Security, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if(NullDevice == INVALID_HANDLE_VALUE)
		{
			throw std::runtime_error(FormatWindowsError("unable to open NUL device"));
		}

		HANDLE ReadPipe = NULL;
		HANDLE WritePipe = NULL;
		if(StandardOutput)
		{
			if(!CreatePipe(&ReadPipe, &WritePipe, &Security, 0))
			{
				const std::string Error = FormatWindowsError("unable to create FFprobe output pipe");
				CloseHandle(NullDevice);
				throw std::runtime_error(Error);
			}
			if(!SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0))
			{
				const std::string Error = FormatWindowsError("unable to configure FFprobe output pipe");
				CloseHandle(ReadPipe);
				CloseHandle(WritePipe);
				CloseHandle(NullDevice);
				throw std::runtime_error(Error);
			}
		}

		STARTUPINFOW Startup;
		ZeroMemory(&Startup, sizeof(Startup));
		Startup.cb = sizeof(Startup);
		Startup.dwFlags = STARTF_USESTDHANDLES;
		Startup.hStdInput = NullDevice;
		Startup.hStdOutput = StandardOutput ? WritePipe : NullDevice;
		Startup.hStdError = NullDevice;
		std::wstring Command = MakeCommandLine(Executable, Arguments);
		PROCESS_INFORMATION Process;
		ZeroMemory(&Process, sizeof(Process));
		const BOOL Started = CreateProcessW(Executable.c_str(), &Command[0], NULL, NULL,
			TRUE, CREATE_NO_WINDOW, NULL, NULL, &Startup, &Process);
		const DWORD StartError = GetLastError();
		if(WritePipe)
		{
			CloseHandle(WritePipe);
		}
		CloseHandle(NullDevice);
		if(!Started)
		{
			if(ReadPipe)
			{
				CloseHandle(ReadPipe);
			}
			throw std::runtime_error("unable to launch FFmpeg/FFprobe (Windows error "
				+ std::to_string(StartError) + ")");
		}

		if(StandardOutput)
		{
			StandardOutput->clear();
			char Buffer[1024];
			DWORD BytesRead = 0;
			while(ReadFile(ReadPipe, Buffer, sizeof(Buffer), &BytesRead, NULL) && BytesRead)
			{
				StandardOutput->append(Buffer, BytesRead);
			}
			CloseHandle(ReadPipe);
		}
		WaitForSingleObject(Process.hProcess, INFINITE);
		DWORD ExitCode = 1;
		GetExitCodeProcess(Process.hProcess, &ExitCode);
		CloseHandle(Process.hThread);
		CloseHandle(Process.hProcess);
		return ExitCode;
	}

	double ProbeDuration(const std::wstring& FfprobePath, const std::wstring& InputPath)
	{
		std::vector<std::wstring> Arguments;
		Arguments.push_back(L"-v");
		Arguments.push_back(L"error");
		Arguments.push_back(L"-select_streams");
		Arguments.push_back(L"a:0");
		Arguments.push_back(L"-show_entries");
		Arguments.push_back(L"stream=duration");
		Arguments.push_back(L"-of");
		Arguments.push_back(L"default=noprint_wrappers=1:nokey=1");
		Arguments.push_back(InputPath);
		std::string Output;
		DWORD ExitCode=RunProcess(FfprobePath, Arguments, &Output);
		char* End=NULL;
		double Duration=ExitCode==0 ? strtod(Output.c_str(), &End) : 0;
		if(End==Output.c_str() || !std::isfinite(Duration) || Duration<=0)
		{
			Arguments[5]=L"format=duration";
			Output.clear();
			ExitCode=RunProcess(FfprobePath, Arguments, &Output);
			End=NULL;
			Duration=ExitCode==0 ? strtod(Output.c_str(), &End) : 0;
		}
		if(End == Output.c_str() || !std::isfinite(Duration) || Duration <= 0)
		{
			throw std::runtime_error("FFprobe could not read a valid source audio duration");
		}
		return Duration;
	}

	std::wstring FormatSeconds(double Seconds)
	{
		wchar_t Buffer[64];
		if(swprintf_s(Buffer, sizeof(Buffer) / sizeof(Buffer[0]), L"%.9f", Seconds) < 0)
		{
			throw std::runtime_error("unable to format audio seek time");
		}
		return Buffer;
	}

	std::wstring MakeAtempoFilter(double Tempo)
	{
		if(!std::isfinite(Tempo) || Tempo <= 0)
		{
			throw std::runtime_error("calculated time-stretch factor is invalid");
		}
		std::wstring Filter;
		while(Tempo > 2.0)
		{
			if(!Filter.empty())
			{
				Filter.append(L",");
			}
			Filter.append(L"atempo=2");
			Tempo /= 2.0;
		}
		while(Tempo < 0.5)
		{
			if(!Filter.empty())
			{
				Filter.append(L",");
			}
			Filter.append(L"atempo=0.5");
			Tempo /= 0.5;
		}
		if(!Filter.empty())
		{
			Filter.append(L",");
		}
		Filter.append(L"atempo=");
		Filter.append(FormatSeconds(Tempo));
		return Filter;
	}
}

std::wstring FindAudioToolOnPath(const std::wstring& Filename)
{
	const DWORD Required=SearchPathW(NULL, Filename.c_str(), NULL, 0, NULL, NULL);
	if(!Required)
	{
		return std::wstring();
	}
	std::vector<wchar_t> Path(Required+1);
	const DWORD Length=SearchPathW(NULL, Filename.c_str(), NULL,
		static_cast<DWORD>(Path.size()), &Path[0], NULL);
	if(!Length || Length>=Path.size())
	{
		return std::wstring();
	}
	return std::wstring(&Path[0], Length);
}

void ConvertAudioToSc1ReplacementWave(const std::wstring& FfmpegPath,
	const std::wstring& FfprobePath, const std::wstring& InputPath,
	const std::wstring& OutputPath, unsigned long SampleRate,
	unsigned long SampleCount, double StartSeconds, EAudioFitMode FitMode)
{
	if(FfmpegPath.empty() || InputPath.empty() || OutputPath.empty())
	{
		throw std::runtime_error("FFmpeg, input audio, and output WAV paths are required");
	}
	if(!SampleRate || !SampleCount || !std::isfinite(StartSeconds) || StartSeconds < 0)
	{
		throw std::runtime_error("invalid target audio format or source start time");
	}

	std::wstring Filter = L"aresample=" + std::to_wstring(SampleRate);
	if(FitMode == EAFM_TIME_STRETCH)
	{
		if(FfprobePath.empty())
		{
			throw std::runtime_error("time-stretch mode requires ffprobe.exe beside ffmpeg.exe or on PATH");
		}
		const double SourceDuration = ProbeDuration(FfprobePath, InputPath) - StartSeconds;
		if(SourceDuration <= 0)
		{
			throw std::runtime_error("source start time is at or beyond the end of the audio");
		}
		const double TargetDuration = static_cast<double>(SampleCount) / SampleRate;
		Filter.append(L",");
		Filter.append(MakeAtempoFilter(SourceDuration / TargetDuration));
	}
	Filter.append(L",apad,atrim=end_sample=");
	Filter.append(std::to_wstring(SampleCount));

	std::vector<std::wstring> Arguments;
	Arguments.push_back(L"-nostdin");
	Arguments.push_back(L"-hide_banner");
	Arguments.push_back(L"-loglevel");
	Arguments.push_back(L"error");
	Arguments.push_back(L"-y");
	Arguments.push_back(L"-ss");
	Arguments.push_back(FormatSeconds(StartSeconds));
	Arguments.push_back(L"-i");
	Arguments.push_back(InputPath);
	Arguments.push_back(L"-map");
	Arguments.push_back(L"0:a:0");
	Arguments.push_back(L"-vn");
	Arguments.push_back(L"-sn");
	Arguments.push_back(L"-dn");
	Arguments.push_back(L"-af");
	Arguments.push_back(Filter);
	Arguments.push_back(L"-ac");
	Arguments.push_back(L"1");
	Arguments.push_back(L"-ar");
	Arguments.push_back(std::to_wstring(SampleRate));
	Arguments.push_back(L"-c:a");
	Arguments.push_back(L"pcm_s16le");
	Arguments.push_back(L"-f");
	Arguments.push_back(L"wav");
	Arguments.push_back(OutputPath);

	if(RunProcess(FfmpegPath, Arguments, NULL) != 0)
	{
		throw std::runtime_error("FFmpeg failed to convert the selected audio file");
	}
}
