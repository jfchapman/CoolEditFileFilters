#include "FFmpegEncoderAAC.h"

#include <set>

extern "C"
{
#include "libavcodec/avcodec.h"
}

FFmpegEncoderAAC::FFmpegEncoderAAC() : FFmpegEncoder()
{
}

const AVCodec* FFmpegEncoderAAC::GetCodec() const
{
	return avcodec_find_encoder( AV_CODEC_ID_AAC );
}

bool FFmpegEncoderAAC::SetupCodecContext( AVCodecContext* codecContext, const AVCodec* codec, const long sampleRate, const long channels, const long bitsPerSample, const int bitrate ) const
{
	codecContext->sample_rate = 0;
	int numSupportedSampleRates = 0;
	const int* pSupportedSampleRates = nullptr;
	if ( avcodec_get_supported_config( codecContext, codec, AV_CODEC_CONFIG_SAMPLE_RATE, 0, reinterpret_cast<const void**>( &pSupportedSampleRates ), &numSupportedSampleRates ) >= 0 ) {
		std::set<int> supportedSampleRates;
		supportedSampleRates.insert( pSupportedSampleRates, pSupportedSampleRates + numSupportedSampleRates );
		for ( const auto supportedSampleRate : supportedSampleRates ) {
			if ( supportedSampleRate >= sampleRate ) {
				codecContext->sample_rate = supportedSampleRate;
				break;
			}
		}
		if ( 0 == codecContext->sample_rate ) {
			codecContext->sample_rate = *supportedSampleRates.rbegin();
		}
	}
	if ( codecContext->sample_rate > 0 ) {
		codecContext->ch_layout = GetChannelLayout( channels );
		codecContext->bit_rate = 1000 * bitrate;
		codecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
		return true;
	}
	return false;
}
