#include "FFmpegAacFileFilter.h"
#include "FFmpegDecoder.h"
#include "FFmpegEncoderAAC.h"
#include "utils.h"
#include "resource.h"

#include <algorithm>
#include <stdexcept>

constexpr long kChunkSize = 65536;

constexpr int kAACMinimumBitrate = 64;
constexpr int kAACMaximumBitrate = 320;
constexpr int kAACDefaultBitrate = 128;

constexpr char kBitrateSetting[] = "aacBitrate";

int __stdcall QueryCoolFilter( COOLQUERY* cq )
{
  _ASSERT_EXPR( 0, L"QueryCoolFilter" );

  strcpy_s( cq->szExt, 4, "M4A" );		
	strcpy_s( cq->szName, 24, "AAC" );		
	strcpy_s( cq->szCopyright, 80, FFmpegDecoder::GetVersion().c_str() );
	cq->lChunkSize = 0; 
	cq->dwFlags = QF_CANLOAD | QF_CANSAVE | QF_RATEADJUSTABLE | QF_HASOPTIONSBOX | QF_CANDO32BITFLOATS | QF_READSPECIALFIRST | QF_WRITESPECIALFIRST;
 	cq->Stereo8 = 0xFF;
 	cq->Stereo16 = 0xFF;
 	cq->Stereo32 = 0xFF;
 	cq->Mono8 = 0xFF;
 	cq->Mono16 = 0xFF;
 	cq->Mono32 = 0xFF;
  return C_VALIDLIBRARY;
}

BOOL __stdcall FilterUnderstandsFormat( LPSTR filename )
{
  try {
    FFmpegDecoder decoder( AnsiCodePageToUTF8( filename ), AV_CODEC_ID_AAC );
    return TRUE;
  } catch ( const std::runtime_error& ) {
    return FALSE;
  }
}

HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND, LONG* chunkSize )
{
  try {
    auto decoder = new FFmpegDecoder( AnsiCodePageToUTF8( filename ), AV_CODEC_ID_AAC );
    if ( nullptr != sampleRate )
      *sampleRate = static_cast<LONG>( decoder->GetSampleRate() );
    if ( nullptr != bitsPerSample )
      *bitsPerSample = static_cast<WORD>( decoder->GetBitsPerSample() );
    if ( nullptr != channels )
      *channels = static_cast<WORD>( decoder->GetChannels() );
    if ( nullptr != chunkSize )
      *chunkSize = kChunkSize;
    return decoder;
  } catch ( const std::runtime_error& ) {
    return 0;
  }
}

DWORD __stdcall FilterGetFileSize( HANDLE hInput )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;

  const uint64_t totalBytes = decoder->GetTotalSamples() * decoder->GetChannels() * decoder->GetBitsPerSample() / 8;
  return static_cast<DWORD>( std::min( totalBytes, static_cast<uint64_t>( std::numeric_limits<DWORD>::max() ) ) );
}

DWORD __stdcall ReadFilterInput( HANDLE hInput, BYTE* data, LONG bytes )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;
  return decoder->Read( data, bytes );
}

void __stdcall CloseFilterInput( HANDLE hInput )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr != decoder )
    delete decoder;
}

void __stdcall GetSuggestedSampleType( LONG* sampleRate, WORD* bitsPerSample, WORD* channels )
{
  *sampleRate = 0;
  *bitsPerSample = 0;
  *channels = 0;
}

HANDLE __stdcall OpenFilterOutput( LPSTR filename, LONG sampleRate, WORD bitsPerSample, WORD channels, LONG size, LONG* chunkSize, DWORD options )
{
	const std::string utf8Filename = AnsiCodePageToUTF8( filename );
  FFmpegEncoder* encoder = new FFmpegEncoderAAC();
  *chunkSize = static_cast<LONG>( kChunkSize );
	const int aacBitrate = ReadSetting( kBitrateSetting ).value_or( kAACDefaultBitrate );
	if ( nullptr != encoder && !encoder->Open( utf8Filename, static_cast<uint32_t>( sampleRate ), static_cast<uint32_t>( bitsPerSample ), static_cast<uint32_t>( channels ), aacBitrate ) ) {
		delete encoder;
		encoder = nullptr;
	}	
	return encoder;
}

void __stdcall CloseFilterOutput( HANDLE output )
{
  FFmpegEncoder* encoder = static_cast<FFmpegEncoder*>( output );
  if ( nullptr != encoder )
    delete encoder;
}

DWORD __stdcall WriteFilterOutput( HANDLE output, BYTE* data, LONG bytes)
{
  FFmpegEncoder* encoder = static_cast<FFmpegEncoder*>( output );
  if ( nullptr == encoder )
    return 0;
  return encoder->Write( data, bytes );
}

INT_PTR CALLBACK DialogProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message ) {
		case WM_INITDIALOG : {
      const int32_t bitrate = std::clamp( ReadSetting( kBitrateSetting ).value_or( kAACDefaultBitrate ), static_cast<int32_t>( kAACMinimumBitrate ), static_cast<int32_t>( kAACMaximumBitrate ) );
      const auto str = std::to_wstring( bitrate );
      SetDlgItemText( hwnd, IDC_BITRATE, str.c_str() );
      return TRUE;
		}
    case WM_COMMAND : {
			switch ( LOWORD( wParam ) ) { 
				case IDOK : {
          try {
						int32_t result = 0;
            std::vector<wchar_t> buffer( 16 );
            if ( GetDlgItemText( hwnd, IDC_BITRATE, buffer.data(), buffer.size() ) )
              result = std::clamp( std::stoi( buffer.data() ), kAACMinimumBitrate, kAACMaximumBitrate );
            WriteSetting( kBitrateSetting, result );
          } catch ( const std::logic_error& ) { }
          EndDialog( hwnd, 1 );
          return TRUE;
        }
				case IDCANCEL :
					EndDialog( hwnd, 0 );
					return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

DWORD __stdcall FilterGetOptions( HWND hwnd, HINSTANCE inst, LONG sampleRate, WORD channels, WORD bitsPerSample, DWORD options )
{
	return DialogBoxParam( inst, MAKEINTRESOURCE(IDD_CONFIG), hwnd, DialogProc, options );
}

DWORD __stdcall FilterOptions( HANDLE hInput )
{
  return 0;
}

DWORD __stdcall FilterOptionsString( HANDLE hInput, LPSTR str )
{
  constexpr uint32_t kMaxStringLength = 80;
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr != decoder ) {
    // We don't know the length of the destination buffer, so put a sensible limit on our string length.
    const uint32_t count = std::min( decoder->GetDescription().size(), kMaxStringLength - 1 );
    memcpy( str, decoder->GetDescription().c_str(), count );
    str[ count ] = 0;
  }
  return 0;
}

DWORD __stdcall FilterWriteSpecialData( HANDLE output, LPCSTR listType, LPCSTR type, char* data, DWORD size )
{
  return 0;
}

DWORD __stdcall FilterGetFirstSpecialData( HANDLE hInput, SPECIALDATA* specialData )
{
  return 0;
}

DWORD __stdcall FilterGetNextSpecialData( HANDLE hInput, SPECIALDATA* specialData )
{
  return 0;
}
