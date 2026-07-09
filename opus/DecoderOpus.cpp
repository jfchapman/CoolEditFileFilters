#include "DecoderOpus.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

DecoderOpus::DecoderOpus( const std::string& filename )
{
	int error = 0;
	m_OpusFile = op_open_file( filename.c_str(), &error );
	if ( nullptr != m_OpusFile ) {
		if ( const OpusHead* head = op_head( m_OpusFile, -1 ); nullptr != head ) {
      m_SampleRate = 48000;
      m_Channels = std::min( 2u, static_cast<uint32_t>( head->channel_count ) );
      m_Bitrate = static_cast<uint32_t>( op_bitrate( m_OpusFile, -1 ) );
      m_TotalSamples = static_cast<uint64_t>( op_pcm_total( m_OpusFile, -1 ) );

      // Use 32-bit samples if this will fit within Cool Edit's 2Gb limit.
      m_BitsPerSample = ( ( m_TotalSamples * m_Channels * 32 / 8 ) <= std::numeric_limits<int>().max() ) ? 32u : 16u;
		}

    if ( const OpusTags* tags = op_tags( m_OpusFile, -1 ); nullptr != tags ) {
      if ( nullptr != tags->vendor )
        m_Description = tags->vendor;
      for ( int i = 0; i < tags->comments; i++ ) {
        const std::string comment( tags->user_comments[ i ], tags->user_comments[ i ] + tags->comment_lengths[ i ] );
        if ( const auto pos = comment.find( '=' ); std::string::npos != pos ) {
          auto name = comment.substr( 0, pos );
          const auto value = comment.substr( 1 + pos );
          if ( !name.empty() && !value.empty() ) {
            std::transform( name.begin(), name.end(), name.begin(), [] ( unsigned char c ) { return std::tolower(c); } );
            m_Tags.insert( { name, value } );
          }
        }
      }
    }

    if ( m_Description.empty() )
      m_Description = "Opus";
    if ( m_Bitrate > 0 )
      m_Description += std::string( "\n" ) + std::to_string( m_Bitrate / 1000 ) + " kbps, ";
    m_Description += std::to_string( m_Channels ) + std::string( 1 == m_Channels ? " channel" : " channels" );
	} else {
		throw std::runtime_error( "OpusDecoder could not load file" );
	}
  m_OpusBuffer.resize( kOpusChunkSize );
}

DecoderOpus::~DecoderOpus()
{
	if ( nullptr != m_OpusFile )
		op_free( m_OpusFile );
}

uint32_t DecoderOpus::Read( unsigned char* destBuffer, const long byteCount )
{
  const uint32_t sampleCount = static_cast<uint32_t>( byteCount ) / m_Channels / ( m_BitsPerSample / 8 );
	uint32_t samplesRead = 0;
  while ( samplesRead < sampleCount ) {
    if ( m_OpusBufferPos < m_OpusBufferSize ) {
      const uint32_t bytesToCopy = std::min( ( sampleCount - samplesRead ) * m_Channels * m_BitsPerSample / 8, m_OpusBufferSize - m_OpusBufferPos );
      std::copy( m_OpusBuffer.begin() + m_OpusBufferPos, m_OpusBuffer.begin() + m_OpusBufferPos + bytesToCopy, destBuffer );
      m_OpusBufferPos += bytesToCopy;
      destBuffer += bytesToCopy;
      samplesRead += bytesToCopy / m_Channels / ( m_BitsPerSample / 8 );
    } else {
      int samples = 0;
      switch ( m_BitsPerSample ) {
        case 16: {
          short* pcmBuffer = reinterpret_cast<short*>( m_OpusBuffer.data() );
          const int pcmSize = m_OpusBuffer.size() / ( m_BitsPerSample / 8 );
          samples = ( 1 == m_Channels ) ? op_read( m_OpusFile, pcmBuffer, pcmSize, nullptr ) : op_read_stereo( m_OpusFile, pcmBuffer, pcmSize );
          break;
        }
        case 32: {
          float* pcmBuffer = reinterpret_cast<float*>( m_OpusBuffer.data() );
          const int pcmSize = m_OpusBuffer.size() / ( m_BitsPerSample / 8 );
          samples = ( 1 == m_Channels ) ? op_read_float( m_OpusFile, pcmBuffer, pcmSize, nullptr ) : op_read_float_stereo( m_OpusFile, pcmBuffer, pcmSize );
          for ( int i = 0; i < samples * static_cast<int>( m_Channels ); i++ )
            pcmBuffer[ i ] *= 32768.f;
          break;
        }
      }
      m_OpusBufferPos = 0;
      m_OpusBufferSize = std::max( samples, 0 ) * m_Channels * m_BitsPerSample / 8;
      if ( 0 == m_OpusBufferSize )
        break;
    }
	}
	return samplesRead * m_Channels * m_BitsPerSample / 8;
}

std::optional<std::string> DecoderOpus::GetTagValue( const std::string& name ) const
{
  std::string tagName( name );
  std::transform( tagName.begin(), tagName.end(), tagName.begin(), [] ( unsigned char c ) { return std::tolower(c); } );
  if ( const auto tag = m_Tags.find( tagName ); m_Tags.end() != tag )
    return tag->second;
  return std::nullopt;
}
