#pragma once

#include <string>
#include <vector>

struct AVCodecContext;
struct AVCodec;
struct AVFormatContext;
struct AVChannelLayout;
struct SwrContext;

class FFmpegEncoder
{
public:
	FFmpegEncoder();
  virtual ~FFmpegEncoder();

	bool Open( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const int bitrate = 0 );
	uint32_t Write( unsigned char* buffer, const long byteCount );

protected:

	virtual const AVCodec* GetCodec() const = 0;
	virtual bool SetupCodecContext( AVCodecContext* codecContext, const AVCodec* codec, const long sampleRate, const long channels, const long bitsPerSample, const int bitrate ) const = 0;

	static AVChannelLayout GetChannelLayout( const uint32_t channels );

private:
	uint32_t Resample( const uint8_t* const inputBuffer, const uint32_t inputSamples, uint8_t** outputBuffers, const uint32_t outputSamples );
	bool EncodeSamples( uint8_t* inputSamples, const long inputSampleCount );
	bool EncodeFrame();
	void FreeContexts();

	AVFormatContext* m_avformatContext = nullptr;
	AVCodecContext* m_avcodecContext = nullptr;
	SwrContext* m_swrContext = nullptr;
	int64_t m_pts = 0;
	uint32_t m_InputChannels = 0;
	uint32_t m_InputBytesPerSample = 0;
	uint32_t m_OutputBytesPerSample = 0;
	int m_InputSampleRate = 0;
	int m_OutputSampleRate = 0;
	int m_OutputChannels = 0;
	std::vector<std::vector<uint8_t>> m_SampleBuffers;
};
