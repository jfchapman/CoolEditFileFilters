#pragma once

#include "FFmpegEncoder.h"

class FFmpegEncoderAAC : public FFmpegEncoder
{
public:
	FFmpegEncoderAAC();

protected:
	const AVCodec* GetCodec() const override;
	bool SetupCodecContext( AVCodecContext* codecContext, const AVCodec* codec, const long sampleRate, const long channels, const long bitsPerSample, const int bitrate ) const override;
};
