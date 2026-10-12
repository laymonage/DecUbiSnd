#include "Pch.h"

#define NOMINMAX
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
}

#include "AudioConverter.h"

namespace
{
	void LoadFFmpegLibraries()
	{
		const char* Libraries[] =
		{
			"avutil-61.dll",
			"avcodec-63.dll",
			"avformat-63.dll",
			"avfilter-12.dll"
		};
		static HMODULE LoadedLibraries[sizeof(Libraries) / sizeof(Libraries[0])] = {};
		static std::once_flag LoadFlag;
		std::call_once(LoadFlag, [&]()
		{
			for(size_t Index = 0; Index < sizeof(Libraries) / sizeof(Libraries[0]); ++Index)
			{
				if(!LoadedLibraries[Index])
				{
					LoadedLibraries[Index] = LoadLibraryA(Libraries[Index]);
					if(!LoadedLibraries[Index])
					{
						const DWORD ErrorCode = GetLastError();
						std::ostringstream Message;
						Message << "unable to load FFmpeg library " << Libraries[Index]
							<< " or one of its dependencies; ensure the bundled FFmpeg DLLs "
							<< "are present beside the executable (Windows error "
							<< ErrorCode << ")";
						throw std::runtime_error(Message.str());
					}
				}
			}
		});
	}

	struct SFFmpegResources
	{
		AVFormatContext* Input = NULL;
		AVCodecContext* Decoder = NULL;
		AVPacket* Packet = NULL;
		AVFrame* DecodedFrame = NULL;
		AVFrame* FilterFrame = NULL;
		AVFilterGraph* FilterGraph = NULL;
		AVFilterContext* FilterSource = NULL;
		AVFilterContext* FilterSink = NULL;
		AVFilterInOut* FilterInputs = NULL;
		AVFilterInOut* FilterOutputs = NULL;

		~SFFmpegResources()
		{
			avfilter_inout_free(&FilterInputs);
			avfilter_inout_free(&FilterOutputs);
			av_frame_free(&FilterFrame);
			av_frame_free(&DecodedFrame);
			av_packet_free(&Packet);
			avcodec_free_context(&Decoder);
			avformat_close_input(&Input);
			avfilter_graph_free(&FilterGraph);
		}
	};

	std::string GetFfmpegError(int ErrorCode)
	{
		char Message[AV_ERROR_MAX_STRING_SIZE];
		if(av_strerror(ErrorCode, Message, sizeof(Message)) < 0)
		{
			return "unknown FFmpeg error";
		}
		return Message;
	}

	void CheckFfmpeg(int ErrorCode, const char* Action)
	{
		if(ErrorCode < 0)
		{
			throw std::runtime_error(std::string(Action) + ": "
				+ GetFfmpegError(ErrorCode));
		}
	}

	std::string WidePathToUtf8(const std::wstring& Path)
	{
		const int Required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			Path.c_str(), -1, NULL, 0, NULL, NULL);
		if(!Required)
		{
			throw std::runtime_error("unable to encode the input path as UTF-8");
		}
		std::string Utf8(static_cast<size_t>(Required), '\0');
		if(!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Path.c_str(), -1,
			&Utf8[0], Required, NULL, NULL))
		{
			throw std::runtime_error("unable to encode the input path as UTF-8");
		}
		Utf8.resize(static_cast<size_t>(Required - 1));
		return Utf8;
	}

	class CWaveWriter
	{
	public:
		CWaveWriter(const std::wstring& Path, unsigned long SampleRate,
			unsigned long SampleCount)
			: m_File(NULL), m_SampleRate(SampleRate), m_SampleCount(SampleCount),
			m_WrittenSamples(0)
		{
			if(SampleCount > (std::numeric_limits<uint32_t>::max() - 36U) / 2U)
			{
				throw std::runtime_error("replacement WAV would exceed the RIFF size limit");
			}
			if(_wfopen_s(&m_File, Path.c_str(), L"wb") != 0 || !m_File)
			{
				throw std::runtime_error("unable to create the temporary replacement WAV");
			}
			try
			{
				WriteHeader(0);
			}
			catch(...)
			{
				fclose(m_File);
				m_File = NULL;
				throw;
			}
		}

		~CWaveWriter()
		{
			if(m_File)
			{
				fclose(m_File);
			}
		}

		void WriteSamples(const int16_t* Samples, size_t Count)
		{
			const uint64_t Remaining = m_SampleCount - m_WrittenSamples;
			const size_t ToWrite = static_cast<size_t>(
				Remaining < Count ? Remaining : Count);
			if(ToWrite && fwrite(Samples, sizeof(int16_t), ToWrite, m_File) != ToWrite)
			{
				throw std::runtime_error("unable to write converted audio samples");
			}
			m_WrittenSamples += ToWrite;
		}

		void Finish()
		{
			int16_t Silence[4096] = {};
			while(m_WrittenSamples < m_SampleCount)
			{
				const uint64_t Remaining = m_SampleCount - m_WrittenSamples;
				const size_t ToWrite = static_cast<size_t>(
					Remaining < sizeof(Silence) / sizeof(Silence[0])
						? Remaining : sizeof(Silence) / sizeof(Silence[0]));
				if(fwrite(Silence, sizeof(int16_t), ToWrite, m_File) != ToWrite)
				{
					throw std::runtime_error("unable to pad converted audio samples");
				}
				m_WrittenSamples += ToWrite;
			}

			if(_fseeki64(m_File, 0, SEEK_SET) != 0)
			{
				throw std::runtime_error("unable to finalize the replacement WAV header");
			}
			WriteHeader(static_cast<uint32_t>(m_SampleCount * 2U));
			if(fflush(m_File) != 0)
			{
				throw std::runtime_error("unable to flush the replacement WAV file");
			}
			const int CloseResult = fclose(m_File);
			m_File = NULL;
			if(CloseResult != 0)
			{
				throw std::runtime_error("unable to close the replacement WAV file");
			}
		}

	private:
		static void WriteBytes(FILE* File, const char* Bytes, size_t Count)
		{
			if(fwrite(Bytes, 1, Count, File) != Count)
			{
				throw std::runtime_error("unable to write the replacement WAV header");
			}
		}

		static void WriteUint16(FILE* File, uint16_t Value)
		{
			const unsigned char Bytes[2] = {
				static_cast<unsigned char>(Value),
				static_cast<unsigned char>(Value >> 8)
			};
			WriteBytes(File, reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
		}

		static void WriteUint32(FILE* File, uint32_t Value)
		{
			const unsigned char Bytes[4] = {
				static_cast<unsigned char>(Value),
				static_cast<unsigned char>(Value >> 8),
				static_cast<unsigned char>(Value >> 16),
				static_cast<unsigned char>(Value >> 24)
			};
			WriteBytes(File, reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
		}

		void WriteHeader(uint32_t DataSize)
		{
			WriteBytes(m_File, "RIFF", 4);
			WriteUint32(m_File, 36U + DataSize);
			WriteBytes(m_File, "WAVEfmt ", 8);
			WriteUint32(m_File, 16);
			WriteUint16(m_File, 1);
			WriteUint16(m_File, 1);
			WriteUint32(m_File, static_cast<uint32_t>(m_SampleRate));
			WriteUint32(m_File, static_cast<uint32_t>(m_SampleRate * 2U));
			WriteUint16(m_File, 2);
			WriteUint16(m_File, 16);
			WriteBytes(m_File, "data", 4);
			WriteUint32(m_File, DataSize);
		}

		FILE* m_File;
		unsigned long m_SampleRate;
		unsigned long m_SampleCount;
		uint64_t m_WrittenSamples;
	};

	void DrainFilter(SFFmpegResources& Resources, CWaveWriter& Writer,
		bool RequireEndOfStream = false)
	{
		for(;;)
		{
			const int Result = av_buffersink_get_frame(Resources.FilterSink,
				Resources.FilterFrame);
			if(Result == AVERROR(EAGAIN))
			{
				if(RequireEndOfStream)
				{
					throw std::runtime_error("audio filter stopped before reaching end of stream");
				}
				return;
			}
			if(Result == AVERROR_EOF)
			{
				return;
			}
			CheckFfmpeg(Result, "unable to read filtered audio");
			if(Resources.FilterFrame->format != AV_SAMPLE_FMT_S16
				|| Resources.FilterFrame->ch_layout.nb_channels != 1
				|| Resources.FilterFrame->nb_samples < 0)
			{
				throw std::runtime_error("FFmpeg produced audio in an unexpected format");
			}
			Writer.WriteSamples(
				reinterpret_cast<const int16_t*>(Resources.FilterFrame->extended_data[0]),
				static_cast<size_t>(Resources.FilterFrame->nb_samples));
			av_frame_unref(Resources.FilterFrame);
		}
	}

	int DrainDecoder(SFFmpegResources& Resources, CWaveWriter& Writer,
		int64_t& NextPts)
	{
		int FramesDecoded = 0;
		for(;;)
		{
			const int Result = avcodec_receive_frame(Resources.Decoder,
				Resources.DecodedFrame);
			if(Result == AVERROR(EAGAIN) || Result == AVERROR_EOF)
			{
				return FramesDecoded;
			}
			CheckFfmpeg(Result, "unable to decode source audio");
			++FramesDecoded;
			if(Resources.DecodedFrame->sample_rate != Resources.Decoder->sample_rate)
			{
				throw std::runtime_error("source audio changes sample rate while decoding");
			}
			if(Resources.DecodedFrame->nb_samples < 0
				|| NextPts > std::numeric_limits<int64_t>::max()
					- Resources.DecodedFrame->nb_samples)
			{
				throw std::runtime_error("source audio is too long to convert");
			}
			Resources.DecodedFrame->pts = NextPts;
			NextPts += Resources.DecodedFrame->nb_samples;
			CheckFfmpeg(av_buffersrc_add_frame_flags(Resources.FilterSource,
				Resources.DecodedFrame, AV_BUFFERSRC_FLAG_KEEP_REF),
				"unable to pass decoded audio to the filter");
			DrainFilter(Resources, Writer);
			av_frame_unref(Resources.DecodedFrame);
		}
	}

	void SendPacket(SFFmpegResources& Resources, const AVPacket* Packet,
		CWaveWriter& Writer, int64_t& NextPts)
	{
		for(;;)
		{
			const int Result = avcodec_send_packet(Resources.Decoder, Packet);
			if(Result == AVERROR(EAGAIN))
			{
				if(DrainDecoder(Resources, Writer, NextPts) > 0)
				{
					continue;
				}
				throw std::runtime_error("audio decoder made no progress while sending a packet");
			}
			CheckFfmpeg(Result, "unable to submit source audio for decoding");
			break;
		}
		DrainDecoder(Resources, Writer, NextPts);
	}

	std::string MakeAtempoFilter(double Tempo)
	{
		if(!std::isfinite(Tempo) || Tempo <= 0)
		{
			throw std::runtime_error("calculated time-stretch factor is invalid");
		}
		std::ostringstream Filter;
		Filter.imbue(std::locale::classic());
		size_t FilterCount = 0;
		while(Tempo > 2.0)
		{
			Filter << (FilterCount++ ? "," : "") << "atempo=2";
			Tempo /= 2.0;
		}
		while(Tempo < 0.5)
		{
			Filter << (FilterCount++ ? "," : "") << "atempo=0.5";
			Tempo /= 0.5;
		}
		Filter << (FilterCount++ ? "," : "") << "atempo=" << Tempo;
		return Filter.str();
	}

	void ConfigureFilterGraph(SFFmpegResources& Resources,
		unsigned long SampleRate, unsigned long SampleCount,
		int64_t StartSample, const std::string& TempoFilter)
	{
		Resources.FilterGraph = avfilter_graph_alloc();
		if(!Resources.FilterGraph)
		{
			throw std::runtime_error("unable to allocate the audio filter graph");
		}

		const AVFilter* BufferFilter = avfilter_get_by_name("abuffer");
		const AVFilter* SinkFilter = avfilter_get_by_name("abuffersink");
		if(!BufferFilter || !SinkFilter)
		{
			throw std::runtime_error("FFmpeg audio buffer filters are unavailable");
		}

		if(Resources.Decoder->ch_layout.nb_channels <= 0)
		{
			throw std::runtime_error("source audio has no channel layout");
		}
		if(Resources.Decoder->ch_layout.order != AV_CHANNEL_ORDER_NATIVE)
		{
			av_channel_layout_default(&Resources.Decoder->ch_layout,
				Resources.Decoder->ch_layout.nb_channels);
		}

		std::ostringstream SourceArguments;
		SourceArguments.imbue(std::locale::classic());
		SourceArguments << "time_base=1/" << Resources.Decoder->sample_rate
			<< ":sample_rate=" << Resources.Decoder->sample_rate
			<< ":sample_fmt="
			<< av_get_sample_fmt_name(Resources.Decoder->sample_fmt)
			<< ":channel_layout=0x" << std::hex
			<< Resources.Decoder->ch_layout.u.mask;
		CheckFfmpeg(avfilter_graph_create_filter(&Resources.FilterSource,
			BufferFilter, "in", SourceArguments.str().c_str(), NULL,
			Resources.FilterGraph), "unable to create the audio filter source");
		CheckFfmpeg(avfilter_graph_create_filter(&Resources.FilterSink,
			SinkFilter, "out", NULL, NULL, Resources.FilterGraph),
			"unable to create the audio filter output");

		Resources.FilterOutputs = avfilter_inout_alloc();
		Resources.FilterInputs = avfilter_inout_alloc();
		if(!Resources.FilterOutputs || !Resources.FilterInputs)
		{
			throw std::runtime_error("unable to allocate audio filter connections");
		}
		Resources.FilterOutputs->name = av_strdup("in");
		Resources.FilterOutputs->filter_ctx = Resources.FilterSource;
		Resources.FilterOutputs->pad_idx = 0;
		Resources.FilterInputs->name = av_strdup("out");
		Resources.FilterInputs->filter_ctx = Resources.FilterSink;
		Resources.FilterInputs->pad_idx = 0;
		if(!Resources.FilterOutputs->name || !Resources.FilterInputs->name)
		{
			throw std::runtime_error("unable to name audio filter connections");
		}

		std::ostringstream Description;
		Description << "atrim=start_sample=" << StartSample
			<< ",asetpts=PTS-STARTPTS";
		if(!TempoFilter.empty())
		{
			Description << "," << TempoFilter;
		}
		Description << ",aresample=" << SampleRate
			<< ",aformat=sample_fmts=s16:sample_rates=" << SampleRate
			<< ":channel_layouts=mono,apad=whole_len=" << SampleCount
			<< ",atrim=end_sample=" << SampleCount;
		const std::string FilterDescription = Description.str();
		CheckFfmpeg(avfilter_graph_parse_ptr(Resources.FilterGraph,
			FilterDescription.c_str(), &Resources.FilterInputs,
			&Resources.FilterOutputs, NULL),
			"unable to configure audio conversion filters");
		avfilter_inout_free(&Resources.FilterInputs);
		avfilter_inout_free(&Resources.FilterOutputs);
		CheckFfmpeg(avfilter_graph_config(Resources.FilterGraph, NULL),
			"unable to initialize audio conversion filters");
	}

	double GetSourceDuration(const AVFormatContext* Input, const AVStream* Stream)
	{
		if(Stream->duration != AV_NOPTS_VALUE && Stream->duration > 0)
		{
			return Stream->duration * av_q2d(Stream->time_base);
		}
		if(Input->duration != AV_NOPTS_VALUE && Input->duration > 0)
		{
			return static_cast<double>(Input->duration) / AV_TIME_BASE;
		}
		return 0;
	}
}

void ConvertAudioToSc1ReplacementWave(const std::wstring& InputPath,
	const std::wstring& OutputPath, unsigned long SampleRate,
	unsigned long SampleCount, double StartSeconds, EAudioFitMode FitMode)
{
	if(InputPath.empty() || OutputPath.empty())
	{
		throw std::runtime_error("input audio and output WAV paths are required");
	}
	if(!SampleRate || !SampleCount || !std::isfinite(StartSeconds)
		|| StartSeconds < 0)
	{
		throw std::runtime_error("invalid target audio format or source start time");
	}
	if(FitMode != EAFM_TRIM_PAD && FitMode != EAFM_TIME_STRETCH)
	{
		throw std::runtime_error("invalid audio fit mode");
	}
	if(SampleRate > std::numeric_limits<uint32_t>::max() / 2U)
	{
		throw std::runtime_error("replacement WAV sample rate is too large");
	}
	if(SampleCount > (std::numeric_limits<uint32_t>::max() - 36U) / 2U)
	{
		throw std::runtime_error("replacement WAV would exceed the RIFF size limit");
	}

	LoadFFmpegLibraries();

	SFFmpegResources Resources;
	const std::string InputPathUtf8 = WidePathToUtf8(InputPath);
	CheckFfmpeg(avformat_open_input(&Resources.Input, InputPathUtf8.c_str(),
		NULL, NULL), "unable to open source audio");
	CheckFfmpeg(avformat_find_stream_info(Resources.Input, NULL),
		"unable to read source audio information");

	const AVCodec* Decoder = NULL;
	const int AudioStreamIndex = av_find_best_stream(Resources.Input,
		AVMEDIA_TYPE_AUDIO, -1, -1, &Decoder, 0);
	CheckFfmpeg(AudioStreamIndex, "source file contains no decodable audio stream");
	if(!Decoder)
	{
		throw std::runtime_error("no decoder is available for the source audio format");
	}
	AVStream* AudioStream = Resources.Input->streams[AudioStreamIndex];
	Resources.Decoder = avcodec_alloc_context3(Decoder);
	if(!Resources.Decoder)
	{
		throw std::runtime_error("unable to allocate the source audio decoder");
	}
	CheckFfmpeg(avcodec_parameters_to_context(Resources.Decoder,
		AudioStream->codecpar), "unable to configure the source audio decoder");
	CheckFfmpeg(avcodec_open2(Resources.Decoder, Decoder, NULL),
		"unable to open the source audio decoder");
	if(Resources.Decoder->sample_rate <= 0
		|| Resources.Decoder->sample_fmt == AV_SAMPLE_FMT_NONE)
	{
		throw std::runtime_error("source audio has an invalid sample format");
	}

	const long double StartSamples = static_cast<long double>(StartSeconds)
		* Resources.Decoder->sample_rate;
	if(!std::isfinite(StartSamples)
		|| StartSamples > std::numeric_limits<int64_t>::max() - 0.5L)
	{
		throw std::runtime_error("source start time is too large");
	}
	const int64_t StartSample = static_cast<int64_t>(std::floor(StartSamples + 0.5L));

	std::string TempoFilter;
	if(FitMode == EAFM_TIME_STRETCH)
	{
		const double SourceDuration = GetSourceDuration(Resources.Input, AudioStream)
			- StartSeconds;
		if(!std::isfinite(SourceDuration) || SourceDuration <= 0)
		{
			throw std::runtime_error(
				"source start time is at or beyond the end of the audio, or its duration is unavailable");
		}
		const double TargetDuration = static_cast<double>(SampleCount) / SampleRate;
		TempoFilter = MakeAtempoFilter(SourceDuration / TargetDuration);
	}

	ConfigureFilterGraph(Resources, SampleRate, SampleCount, StartSample,
		TempoFilter);
	Resources.Packet = av_packet_alloc();
	Resources.DecodedFrame = av_frame_alloc();
	Resources.FilterFrame = av_frame_alloc();
	if(!Resources.Packet || !Resources.DecodedFrame || !Resources.FilterFrame)
	{
		throw std::runtime_error("unable to allocate FFmpeg audio buffers");
	}

	CWaveWriter Writer(OutputPath, SampleRate, SampleCount);
	int64_t NextPts = 0;
	int Result = 0;
	while((Result = av_read_frame(Resources.Input, Resources.Packet)) >= 0)
	{
		if(Resources.Packet->stream_index == AudioStreamIndex)
		{
			SendPacket(Resources, Resources.Packet, Writer, NextPts);
		}
		av_packet_unref(Resources.Packet);
	}
	if(Result != AVERROR_EOF)
	{
		CheckFfmpeg(Result, "unable to read source audio packets");
	}
	SendPacket(Resources, NULL, Writer, NextPts);
	CheckFfmpeg(av_buffersrc_add_frame_flags(Resources.FilterSource, NULL, 0),
		"unable to flush audio conversion filters");
	DrainFilter(Resources, Writer, true);
	Writer.Finish();
}
