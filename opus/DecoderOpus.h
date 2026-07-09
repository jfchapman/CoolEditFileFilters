#pragma once
#include "Opusfile.h"

#include <vector>
#include <string>
#include <optional>
#include <map>

constexpr uint32_t kOpusChunkSize = 65536u;

class DecoderOpus
{
public:
	// Throws std::runtime_error if the file could not be loaded.
	DecoderOpus( const std::string& filename );

	virtual ~DecoderOpus();

  uint32_t GetBitsPerSample() const { return m_BitsPerSample; }
  uint32_t GetChannels() const { return m_Channels; }
  uint32_t GetSampleRate() const { return m_SampleRate; }
  uint64_t GetTotalSamples() const { return m_TotalSamples; }
  std::optional<std::string> GetTagValue( const std::string& name ) const;
  const std::string& GetDescription() const { return m_Description; }

	uint32_t Read( unsigned char* buffer, const long byteCount );

private:
	OggOpusFile* m_OpusFile;
  std::vector<uint8_t> m_OpusBuffer;
	uint32_t m_OpusBufferPos = 0;
  uint32_t m_OpusBufferSize = 0;

  uint32_t m_BitsPerSample = 0;
  uint32_t m_Channels = 0;
  uint32_t m_SampleRate = 0;
  uint32_t m_Bitrate = 0;
  uint64_t m_TotalSamples = 0;
  std::string m_Description;
  std::map<std::string /*name*/, std::string /*value*/> m_Tags;
};
