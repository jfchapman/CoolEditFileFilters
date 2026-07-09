#pragma once

#include <fstream>
#include <vector>
#include <string>

extern "C"
{
#include <libavcodec/codec_id.h>
}

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

class FFmpegDecoder
{
public:
	// Throws std::runtime_error if the file could not be loaded.
	FFmpegDecoder( const std::string& filename, const AVCodecID codecRestriction = AV_CODEC_ID_NONE );

	virtual ~FFmpegDecoder();

  uint32_t GetBitsPerSample() const { return m_BitsPerSample; }
  uint32_t GetChannels() const { return m_Channels; }
  uint32_t GetSampleRate() const { return m_SampleRate; }
  uint64_t GetTotalSamples() const { return m_TotalSamples; }
  const std::string& GetDescription() const { return m_Description; }
  static std::string GetVersion();

	uint32_t Read( unsigned char* buffer, const long byteCount );

private:
	bool Decode();
	void ConvertSampleData( const AVFrame* frame );

	AVFormatContext* m_FormatContext = nullptr;
	AVCodecContext* m_DecoderContext = nullptr;
	AVPacket* m_Packet = nullptr;
	AVFrame* m_Frame = nullptr;
  SwrContext* m_ResampleContext = nullptr;
	int m_StreamIndex = 0;
	std::vector<uint8_t> m_Buffer;
	uint32_t m_BufferPos = 0;

  uint32_t m_BitsPerSample = 0;
  uint32_t m_Channels = 0;
  uint32_t m_SampleRate = 0;
  uint32_t m_BitRate = 0;
  uint64_t m_TotalSamples = 0;
  std::string m_Description;
};
