#include "FFmpegDecoder.h"

#include <algorithm>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

std::string FFmpegDecoder::GetVersion()
{
  return ( nullptr != av_version_info() ) ? std::string( "FFmpeg " ) + av_version_info() : std::string( "FFmpeg" );
}

FFmpegDecoder::FFmpegDecoder( const std::string& filename, const AVCodecID codecRestriction )
{
	m_FormatContext = avformat_alloc_context();
	if ( nullptr != m_FormatContext ) {
		if ( 0 == avformat_open_input( &m_FormatContext, filename.c_str(), nullptr, nullptr ) ) {
			if ( avformat_find_stream_info( m_FormatContext, nullptr ) >= 0 ) {
				const AVCodec* codec = nullptr;
				m_StreamIndex = av_find_best_stream( m_FormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0 );
				if ( ( m_StreamIndex >= 0 ) && ( nullptr != codec ) ) {
					if ( AV_CODEC_ID_NONE == codecRestriction || codecRestriction == codec->id ) {
						if ( AVStream* stream = m_FormatContext->streams[ m_StreamIndex ]; nullptr != stream ) {
							if ( AVCodecParameters* codecParams = stream->codecpar; nullptr != codecParams ) {
								if ( codecParams->ch_layout.nb_channels > 0 ) {
									m_Channels = std::min( 2u, static_cast<uint32_t>( codecParams->ch_layout.nb_channels ) );
									m_SampleRate = static_cast<uint32_t>( codecParams->sample_rate );

									if ( codecParams->bit_rate > 0 )
										m_BitRate = static_cast<uint32_t>( std::lroundf( codecParams->bit_rate / 1000.0f ) );

									const double duration = ( stream->duration > 0 ) ? ( stream->duration * av_q2d( stream->time_base ) ) : ( static_cast<double>( m_FormatContext->duration ) / AV_TIME_BASE );
									m_TotalSamples = static_cast<uint64_t>( std::llround( duration * m_SampleRate ) );

									const int bitsPerSample = std::max( codecParams->bits_per_coded_sample, codecParams->bits_per_raw_sample );
									switch ( bitsPerSample ) {
										case 8:
										case 16:
										case 32:
											m_BitsPerSample = bitsPerSample;
											break;
										default:
											m_BitsPerSample = 32;
											break;
									}
									// Don't use 32-bit samples if this will take us over Cool Edit's 2Gb limit.
									if ( ( 32 == m_BitsPerSample ) && ( m_TotalSamples * m_Channels * 32 / 8 ) > std::numeric_limits<int>().max() )
										m_BitsPerSample = 16;

									if ( nullptr != codec->long_name )
										m_Description = std::string( codec->long_name ) + "\n";
									else
										m_Description = GetVersion() + "\n";
									if ( m_BitRate > 0 )
										m_Description += std::to_string( m_BitRate ) + " kbps, ";
									m_Description += std::to_string( codecParams->ch_layout.nb_channels ) + std::string( ( 1 == codecParams->ch_layout.nb_channels ) ? " channel" : " channels" );

									m_DecoderContext = avcodec_alloc_context3( codec );
									if ( nullptr != m_DecoderContext ) {
										int result = avcodec_parameters_to_context( m_DecoderContext, codecParams );
										if ( result >= 0 )
											result = avcodec_open2( m_DecoderContext, codec, nullptr );
										if ( result >= 0 ) {
											m_Packet = av_packet_alloc();
											m_Frame = av_frame_alloc();

											const auto srcLayout = m_DecoderContext->ch_layout;
											const auto srcFormat = m_DecoderContext->sample_fmt;
											AVChannelLayout dstLayout = {};
											av_channel_layout_default( &dstLayout, static_cast<int>( m_Channels ) );
											AVSampleFormat dstFormat = AV_SAMPLE_FMT_NONE;
											switch ( m_BitsPerSample ) {
												case 8:
													dstFormat = AV_SAMPLE_FMT_U8;
													break;
												case 16:
													dstFormat = AV_SAMPLE_FMT_S16;
													break;
												case 32:
													dstFormat = AV_SAMPLE_FMT_FLT;
													break;
											}
											result = swr_alloc_set_opts2( &m_ResampleContext, &dstLayout, dstFormat, codecParams->sample_rate, &srcLayout, srcFormat, codecParams->sample_rate, 0, nullptr );
											if ( result >= 0 ) {
												if ( swr_init( m_ResampleContext ) < 0 ) {
													swr_free( &m_ResampleContext );
													m_ResampleContext = nullptr;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ( ( nullptr == m_FormatContext ) || ( nullptr == m_DecoderContext ) || ( nullptr == m_Packet ) || ( nullptr == m_Frame ) || ( nullptr == m_ResampleContext ) ) {
		if ( nullptr != m_Packet ) {
			av_packet_free( &m_Packet );
		}
		if ( nullptr != m_Frame ) {
			av_frame_free( &m_Frame );
		}
		if ( nullptr != m_DecoderContext ) {
			avcodec_free_context( &m_DecoderContext );
		}
		if ( nullptr != m_FormatContext ) {
			avformat_close_input( &m_FormatContext );
		}
		if ( nullptr != m_ResampleContext ) {
			swr_free( &m_ResampleContext );
		}
		throw std::runtime_error( "FFmpegDecoder failed to initialise" );
	}
}

FFmpegDecoder::~FFmpegDecoder()
{
	if ( nullptr != m_Packet ) {
		av_packet_free( &m_Packet );
	}
	av_frame_free( &m_Frame );
	avcodec_free_context( &m_DecoderContext );
	avformat_close_input( &m_FormatContext );
  swr_free( &m_ResampleContext );
}

uint32_t FFmpegDecoder::Read( unsigned char* destBuffer, const long byteCount )
{
  const uint32_t sampleCount = static_cast<uint32_t>( byteCount ) / m_Channels / ( m_BitsPerSample / 8 );
	uint32_t samplesRead = 0;
	while ( samplesRead < sampleCount ) {
		if ( m_BufferPos < m_Buffer.size() ) {
      const uint32_t samplesToRead = std::min( ( m_Buffer.size() - m_BufferPos ) / m_Channels / ( m_BitsPerSample / 8 ), sampleCount - samplesRead );
      const auto srcFirst = m_Buffer.begin() + m_BufferPos;
      const auto srcLast = srcFirst + samplesToRead * m_Channels * m_BitsPerSample / 8;
      std::copy( srcFirst, srcLast, destBuffer );
      samplesRead += samplesToRead;
      destBuffer += samplesToRead * m_Channels * m_BitsPerSample / 8;
      m_BufferPos += samplesToRead * m_Channels * m_BitsPerSample / 8;
		} else if ( !Decode() ) {
			break;
		}
	}
	return samplesRead * m_Channels * m_BitsPerSample / 8;
}

bool FFmpegDecoder::Decode()
{
	m_Buffer.clear();
	m_BufferPos = 0;
	while ( ( nullptr != m_Packet ) && m_Buffer.empty() ) {
		if ( av_read_frame( m_FormatContext, m_Packet ) < 0 ) {
			// Flush the decoder.
			av_packet_free( &m_Packet );
			m_Packet = nullptr;
		}
		if ( ( nullptr == m_Packet ) || ( m_StreamIndex == m_Packet->stream_index ) ) {
			int result = avcodec_send_packet( m_DecoderContext, m_Packet );
			while ( result >= 0 ) {
				result = avcodec_receive_frame( m_DecoderContext, m_Frame );
				if ( result >= 0 ) {
					ConvertSampleData( m_Frame );
					av_frame_unref( m_Frame );
				}
			}
		}
		if ( nullptr != m_Packet ) {
			av_packet_unref( m_Packet );
		}
	}
	return !m_Buffer.empty();
}

void FFmpegDecoder::ConvertSampleData( const AVFrame* frame )
{
  const uint32_t previousBufferSize = m_Buffer.size();
  m_Buffer.resize( previousBufferSize + frame->nb_samples * m_Channels * m_BitsPerSample / 8 );
  uint8_t* buffer = m_Buffer.data() + previousBufferSize;
  const int samples = swr_convert( m_ResampleContext, &buffer, frame->nb_samples, frame->data, frame->nb_samples );
  if ( samples > 0 ) {
    m_Buffer.resize( previousBufferSize + samples * m_Channels * m_BitsPerSample / 8 );
    if ( 32 == m_BitsPerSample ) {
      // Floating point audio needs to be rescaled for Cool Edit.
      float* buf = reinterpret_cast<float*>( m_Buffer.data() + previousBufferSize );
      std::for_each( buf, buf + samples * m_Channels, [] ( float& f ) { f *= 32768.f; } );
    }
  } else {
    m_Buffer.resize( previousBufferSize );
  }
}
