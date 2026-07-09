#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <optional>
#include <map>
#include <algorithm>

#include "libopenmpt.hpp"

#include "utils.h"

class OpenMPTDecoder
{
public:
  struct Options {
    static constexpr int32_t kDefaultSampleRate = 48000;
    static constexpr int32_t kDefaultStereoSeparation = 100;
    static constexpr int32_t kMaximumStereoSeparation = 200;
    static constexpr int32_t kDefaultInterpolation = 1;
    static constexpr int32_t kDefaultVolumeRamping = -1;

    Options()
    {
      Read();
    }

    Options( const int32_t sampleRate, const int32_t stereoSeparation, const int32_t interpolation, const int32_t volumeRamping, const int32_t repeatCount ) :
      SampleRate( sampleRate ),
      StereoSeparation( stereoSeparation ),
      Interpolation( interpolation ),
      VolumeRamping( volumeRamping ),
      RepeatCount( repeatCount )
    {
      Write();
    }

    static std::map<int32_t, std::wstring> GetSampleRateOptions()
    {
      return {
        { 22050, L"22050 Hz" },
        { 44100, L"44100 Hz" },
        { 48000, L"48000 Hz" },
        { 96000, L"96000 Hz" }
      };
    }

    static std::map<int32_t, std::wstring> GetInterpolationOptions()
    {
      return {
        { 0, L"Default" },
        { 1, L"None" },
        { 2, L"Linear" },
        { 4, L"Cubic" },
        { 8, L"Windowed Sinc" }
      };
    }

    static std::map<int32_t, std::wstring> GetVolumeRampingOptions()
    {
      return {
        { -1, L"Default" },
        { 0, L"Disabled" },
        { 1, L"Strength 1" },
        { 2, L"Strength 2" },
        { 3, L"Strength 3" },
        { 4, L"Strength 4" },
        { 5, L"Strength 5" },
        { 6, L"Strength 6" },
        { 7, L"Strength 7" },
        { 8, L"Strength 8" },
        { 9, L"Strength 9" },
        { 10, L"Strength 10" }
      };
    }

    int32_t SampleRate = kDefaultSampleRate;
    int32_t StereoSeparation = kDefaultStereoSeparation;
    int32_t Interpolation = kDefaultInterpolation;
    int32_t VolumeRamping = kDefaultVolumeRamping;
    int32_t RepeatCount = 0;
  
  private:
    static constexpr char kSettingSampleRate[] = "openmptSampleRate";
    static constexpr char kSettingStereoSeparation[] = "openmptStereoSeparation";
    static constexpr char kSettingInterpolation[] = "openmptInterpolation";
    static constexpr char kSettingVolumeRamping[] = "openmptVolumeRamping";
    static constexpr char kSettingRepeatCount[] = "openmptRepeatCount";

    void Validate()
    {
      if ( !GetSampleRateOptions().contains( SampleRate ) )
        SampleRate = kDefaultSampleRate;
      if ( !GetInterpolationOptions().contains( Interpolation ) )
        Interpolation = kDefaultInterpolation;
      if ( !GetVolumeRampingOptions().contains( VolumeRamping ) )
        VolumeRamping = kDefaultVolumeRamping;
      StereoSeparation = std::clamp( StereoSeparation, 0, kMaximumStereoSeparation );
      RepeatCount = std::clamp( RepeatCount, 0, 1 );
    }

    void Read()
    {
      SampleRate = ReadSetting( kSettingSampleRate ).value_or( kDefaultSampleRate );
      StereoSeparation = ReadSetting( kSettingStereoSeparation ).value_or( kDefaultStereoSeparation );
      Interpolation = ReadSetting( kSettingInterpolation ).value_or( kDefaultInterpolation );
      VolumeRamping = ReadSetting( kSettingVolumeRamping ).value_or( kDefaultVolumeRamping );
      RepeatCount = ReadSetting( kSettingRepeatCount ).value_or( 0 );
      Validate();
    }

    void Write()
    {
      Validate();
      WriteSetting( kSettingSampleRate, SampleRate );
      WriteSetting( kSettingStereoSeparation, StereoSeparation );
      WriteSetting( kSettingInterpolation, Interpolation );
      WriteSetting( kSettingVolumeRamping, VolumeRamping );
      WriteSetting( kSettingRepeatCount, RepeatCount );
    }
  };

  // Throws openmpt::exception (derived from std::exception) if the file could not be loaded.
  OpenMPTDecoder( const std::wstring& filename, const Options& options = {} );

	virtual ~OpenMPTDecoder();

  uint32_t GetBitsPerSample() const { return m_BitsPerSample; }
  uint32_t GetChannels() const { return m_Channels; }
  uint32_t GetSampleRate() const { return m_SampleRate; }
  uint64_t GetTotalSamples() const { return m_TotalSamples; }
  const std::string& GetDescription() const { return m_Description; }
  static std::string GetVersion();

	uint32_t Read( unsigned char* buffer, const long byteCount );

private:
  std::ifstream m_stream;
  openmpt::module m_module;
  
  std::vector<uint8_t> m_Buffer;
	uint32_t m_BufferPos = 0;

  uint32_t m_BitsPerSample = 0;
  uint32_t m_Channels = 0;
  uint32_t m_SampleRate = 0;
  uint32_t m_BitRate = 0;
  uint64_t m_TotalSamples = 0;
  std::string m_Description;
};
