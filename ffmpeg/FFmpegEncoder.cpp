#include "FFmpegEncoder.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include <libswresample/swresample.h>
}

#include <stdexcept>
#include <algorithm>
#include <map>

FFmpegEncoder::FFmpegEncoder()
{
}

FFmpegEncoder::~FFmpegEncoder()
{
	// Flush the resampler.
	EncodeSamples( nullptr, 0 );

	// Flush the encoder and close the output file.
	avcodec_send_frame( m_avcodecContext, nullptr );
	av_write_trailer( m_avformatContext );

	FreeContexts();
}

bool FFmpegEncoder::Open( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const int bitrate )
{
	if ( AVIOContext* ioContext = nullptr; avio_open( &ioContext, filename.c_str(), AVIO_FLAG_WRITE ) >= 0 ) {
		if ( m_avformatContext = avformat_alloc_context(); nullptr != m_avformatContext ) {
			m_avformatContext->pb = ioContext;
			if ( m_avformatContext->oformat = av_guess_format( nullptr, "out.m4a", nullptr ); nullptr != m_avformatContext->oformat ) {
				if ( m_avformatContext->url = av_strdup( filename.c_str() ); nullptr != m_avformatContext->url ) {
					if ( const AVCodec* avCodec = GetCodec(); nullptr != avCodec ) {
						if ( AVStream* avStream = avformat_new_stream( m_avformatContext, nullptr ); nullptr != avStream ) {
							if ( m_avcodecContext = avcodec_alloc_context3( avCodec ); ( nullptr != m_avcodecContext ) && SetupCodecContext( m_avcodecContext, avCodec, sampleRate, channels, bitsPerSample, bitrate ) ) {
								m_InputSampleRate = static_cast<int>( sampleRate );
								m_OutputSampleRate = m_avcodecContext->sample_rate;
								m_OutputChannels = m_avcodecContext->ch_layout.nb_channels;
								m_OutputBytesPerSample = static_cast<uint32_t>( av_get_bytes_per_sample( m_avcodecContext->sample_fmt ) );
								if ( ( m_OutputSampleRate > 0 ) && ( m_OutputChannels > 0 ) && ( m_OutputBytesPerSample > 0 ) ) {
									m_SampleBuffers.resize( m_OutputChannels );

									m_pts = 0;
									avStream->time_base.den = m_avcodecContext->sample_rate;
									avStream->time_base.num = 1;

									if ( m_avformatContext->oformat->flags & AVFMT_GLOBALHEADER ) {
										m_avformatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
									}

									if ( avcodec_open2( m_avcodecContext, avCodec, nullptr ) >= 0 ) {
										if ( avcodec_parameters_from_context( avStream->codecpar, m_avcodecContext ) >= 0 ) {
											if ( avformat_write_header( m_avformatContext, nullptr ) >= 0 ) {
												const AVChannelLayout srcLayout = GetChannelLayout( channels );
												const AVChannelLayout dstLayout = m_avcodecContext->ch_layout;
												AVSampleFormat srcSampleFormat = AV_SAMPLE_FMT_NONE;
												switch ( bitsPerSample ) {
													case 8 : srcSampleFormat = AV_SAMPLE_FMT_U8; break;
													case 16: srcSampleFormat = AV_SAMPLE_FMT_S16; break;
													case 32: srcSampleFormat = AV_SAMPLE_FMT_FLT; break;
												}
												m_InputBytesPerSample = static_cast<uint32_t>( av_get_bytes_per_sample( srcSampleFormat ) );
												m_InputChannels = channels;
												const AVSampleFormat dstSampleFormat = m_avcodecContext->sample_fmt;
												if ( swr_alloc_set_opts2( &m_swrContext, &dstLayout, dstSampleFormat, m_avcodecContext->sample_rate, &srcLayout, srcSampleFormat, m_InputSampleRate, 0, nullptr ) >= 0 ) {
													av_opt_set( m_swrContext, "filter_type", "kaiser", 0 );
													if ( swr_init( m_swrContext ) < 0 ) {
														swr_free( &m_swrContext );
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
	}
	if ( ( nullptr == m_avformatContext ) || ( nullptr == m_avcodecContext ) || ( nullptr == m_swrContext ) ) {
		FreeContexts();
		return false;
	}
	return true;
}

uint32_t FFmpegEncoder::Write( unsigned char* srcBuffer, const long byteCount )
{
	if ( 4 == m_InputBytesPerSample ) {
    // Floating point input needs to be rescaled to +/- 1.0f.
		float* buf = reinterpret_cast<float*>( srcBuffer );
		std::transform( buf, buf + byteCount / m_InputBytesPerSample, buf, [] ( float f ) { return f / 32768.f; } );
	}
	return EncodeSamples( srcBuffer, byteCount / ( m_InputBytesPerSample * m_InputChannels ) );
}

AVChannelLayout FFmpegEncoder::GetChannelLayout( const uint32_t channels )
{
	const std::map<uint32_t, AVChannelLayout> kChannelLayouts =
	{
		{ 1, AV_CHANNEL_LAYOUT_MONO },
		{ 2, AV_CHANNEL_LAYOUT_STEREO },
		{ 3, AV_CHANNEL_LAYOUT_SURROUND },
		{ 4, AV_CHANNEL_LAYOUT_QUAD },
		{ 5, AV_CHANNEL_LAYOUT_5POINT0 },
		{ 6, AV_CHANNEL_LAYOUT_5POINT1 },
		{ 7, AV_CHANNEL_LAYOUT_6POINT1 },
		{ 8, AV_CHANNEL_LAYOUT_7POINT1 },
		{ 10, AV_CHANNEL_LAYOUT_5POINT1POINT4_BACK },
		{ 12, AV_CHANNEL_LAYOUT_7POINT1POINT4_BACK },
		{ 14, AV_CHANNEL_LAYOUT_9POINT1POINT4_BACK },
	};
	if ( const auto layout = kChannelLayouts.find( channels ); kChannelLayouts.end() != layout )
		return layout->second;
	return AV_CHANNEL_LAYOUT_STEREO;
}

void FFmpegEncoder::FreeContexts()
{
	if ( nullptr != m_avcodecContext ) {
		avcodec_free_context( &m_avcodecContext );
	}
	if ( nullptr != m_avformatContext ) {
		if ( nullptr != m_avformatContext->pb ) {
			avio_closep( &m_avformatContext->pb );
		}
		avformat_free_context( m_avformatContext );
		m_avformatContext = nullptr;
	}
	if ( nullptr != m_swrContext ) {
		swr_free( &m_swrContext );
	}
}

bool FFmpegEncoder::EncodeSamples( uint8_t* inputSamples, const long inputSampleCount )
{
	const bool flushResampler = ( nullptr == inputSamples );
	const uint32_t outputSampleCount = flushResampler ?
		static_cast<uint32_t>( swr_get_out_samples( m_swrContext, inputSampleCount ) ) :
		static_cast<uint32_t>( static_cast<int64_t>( inputSampleCount ) * m_OutputSampleRate / m_InputSampleRate );
	uint64_t bufferSampleCount = m_SampleBuffers[ 0 ].size() / m_OutputBytesPerSample;

	// Resample audio input.
	if ( outputSampleCount > 0 ) {
		std::vector<uint8_t*> outputBuffers( m_OutputChannels );
		for ( int channel = 0; channel < m_OutputChannels; channel++ ) {
			m_SampleBuffers[ channel ].resize( static_cast<size_t>( ( bufferSampleCount + outputSampleCount ) * m_OutputBytesPerSample ) );
			outputBuffers[ channel ] = m_SampleBuffers[ channel ].data() + bufferSampleCount * m_OutputBytesPerSample;
		}
		const uint32_t samplesConverted = Resample( inputSamples, inputSampleCount, outputBuffers.data(), outputSampleCount );
		for ( int channel = 0; channel < m_OutputChannels; channel++ ) {
			m_SampleBuffers[ channel ].resize( static_cast<size_t>( ( bufferSampleCount + samplesConverted ) * m_OutputBytesPerSample ) );
		}
		bufferSampleCount = m_SampleBuffers[ 0 ].size() / m_OutputBytesPerSample;
	}

	// Encode audio frames.
	auto keepEncoding = [ flushResampler, this ] ( const uint64_t sampleCount ) {
		return flushResampler ? ( sampleCount > 0 ) : ( sampleCount >= m_avcodecContext->frame_size );
	};

	bool success = true;
	while ( success && keepEncoding( bufferSampleCount ) ) {
		success = EncodeFrame();
		bufferSampleCount = m_SampleBuffers[ 0 ].size() / m_OutputBytesPerSample;
	}

	return success;
}

bool FFmpegEncoder::EncodeFrame()
{
	bool success = false;
	if ( AVFrame* frame = av_frame_alloc(); nullptr != frame ) {
		frame->nb_samples = std::min( m_avcodecContext->frame_size, static_cast<int>( m_SampleBuffers[ 0 ].size() / m_OutputBytesPerSample ) );
		frame->ch_layout = m_avcodecContext->ch_layout;
		frame->format = m_avcodecContext->sample_fmt;
		frame->sample_rate = m_avcodecContext->sample_rate;
		if ( av_frame_get_buffer( frame, 0 ) >= 0 ) {
			for ( int channel = 0; channel < m_OutputChannels; channel++ ) {
				std::copy( m_SampleBuffers[ channel ].begin(), m_SampleBuffers[ channel ].begin() + frame->nb_samples * m_OutputBytesPerSample, frame->data[ channel ] );
				m_SampleBuffers[ channel ].erase( m_SampleBuffers[ channel ].begin(), m_SampleBuffers[ channel ].begin() + frame->nb_samples * m_OutputBytesPerSample );
			}
			if ( AVPacket* packet = av_packet_alloc(); nullptr != packet ) {
				frame->pts = m_pts;
				m_pts += frame->nb_samples;
				if ( avcodec_send_frame( m_avcodecContext, frame ) >= 0 ) {
					if ( avcodec_receive_packet( m_avcodecContext, packet ) >= 0 ) {
						av_write_frame( m_avformatContext, packet );
					}
					success = true;
				}
				av_packet_free( &packet );
			}
		}
		av_frame_free( &frame );
	}
	return success;
}

uint32_t FFmpegEncoder::Resample( const uint8_t* const inputBuffer, const uint32_t inputSamples, uint8_t** outputBuffers, const uint32_t outputSamples )
{
	const int samplesConverted = swr_convert( m_swrContext, outputBuffers, static_cast<int>( outputSamples ), &inputBuffer, static_cast<int>( inputSamples ) );
	return ( samplesConverted < 0 ) ? 0 : static_cast<uint32_t>( samplesConverted );
}
