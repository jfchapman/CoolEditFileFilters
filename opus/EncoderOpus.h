#pragma once
#include "opusenc.h"

#include <memory>
#include <map>
#include <string>

constexpr uint32_t kOpusMinimumBitrate = 8;
constexpr uint32_t kOpusMaximumBitrate = 256;
constexpr uint32_t kOpusDefaultBitrate = 128;

class EncoderOpus
{
public:
	EncoderOpus( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const uint32_t totalSamples, const uint32_t bitrate );
  virtual ~EncoderOpus();

	uint32_t Write( unsigned char* buffer, const long byteCount );
  void AddTag( const std::string& name, const std::string& value );
  static std::string GetVersion();

private:
  const std::string m_Filename;
  const int m_SampleRate;
  const int m_Channels;
  const int m_Bitrate;
  const uint32_t m_BitsPerSample;
	bool m_FirstPass = true;
	OggOpusEnc* m_OpusEncoder = nullptr;
  std::map<std::string /*name*/, std::string /*value*/> m_Tags;
};
