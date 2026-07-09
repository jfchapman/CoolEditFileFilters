#include "FFmpegEncoderALAC.h"

extern "C"
{
#include "libavcodec/avcodec.h"
}

FFmpegEncoderALAC::FFmpegEncoderALAC() : FFmpegEncoder()
{
}

const AVCodec* FFmpegEncoderALAC::GetCodec() const
{
	return avcodec_find_encoder( AV_CODEC_ID_ALAC );
}

bool FFmpegEncoderALAC::SetupCodecContext( AVCodecContext* codecContext, const AVCodec* codec, const long sampleRate, const long channels, const long bitsPerSample, const int /*bitrate*/ ) const
{
	codecContext->ch_layout = GetChannelLayout( 2 ); // Default to stereo output
	int numSupportedChannelLayouts = 0;
	const AVChannelLayout* pSupportedChannelLayouts = nullptr;
	if ( avcodec_get_supported_config( codecContext, codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT, 0, reinterpret_cast<const void**>( &pSupportedChannelLayouts ), &numSupportedChannelLayouts ) >= 0 ) {
		for ( int i = 0; i < numSupportedChannelLayouts; i++ ) {
			if ( pSupportedChannelLayouts[ i ].nb_channels == channels ) {
				codecContext->ch_layout = pSupportedChannelLayouts[ i ];
				break;
			}
		}
	}
	codecContext->sample_rate = static_cast<int>( sampleRate );
	codecContext->sample_fmt = ( bitsPerSample > 16 ) ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S16P;
	codecContext->compression_level = 2;
	return true;
}
