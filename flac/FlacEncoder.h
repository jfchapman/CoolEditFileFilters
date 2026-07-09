#pragma once

#include "FLAC++/all.h"

#include <memory>
#include <map>
#include <string>
#include <vector>

constexpr uint32_t kDefaultCompressionLevel = 5;
constexpr uint32_t kMaximumCompressionLevel = 8;

class FlacEncoder : public FLAC::Encoder::File
{
public:
	FlacEncoder( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const uint32_t totalSamples, const uint32_t compressionLevel );
  virtual ~FlacEncoder();

	uint32_t Write( unsigned char* buffer, const long byteCount );
  void AddTag( const std::string& name, const std::string& value );

private:
  const std::string m_Filename;
  bool m_FirstPass = true;
	std::vector<FLAC::Metadata::Prototype*> m_Metadata;
  std::map<std::string /*name*/, std::string /*value*/> m_Tags;
	std::unique_ptr<FLAC::Metadata::SeekTable> m_SeekTable;
	std::unique_ptr<FLAC::Metadata::Padding> m_Padding;
	std::unique_ptr<FLAC::Metadata::VorbisComment> m_VorbisComment;
};
