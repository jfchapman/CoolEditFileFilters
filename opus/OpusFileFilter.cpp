#include "OpusFileFilter.h"
#include "DecoderOpus.h"
#include "EncoderOpus.h"
#include "utils.h"
#include "resource.h"

#include <algorithm>
#include <stdexcept>

constexpr char kBitrateSetting[] = "opusBitrate";

static const std::map<std::string /*type*/, std::string /*tag*/> kTypeToTag = {
	{ "IART", "ARTIST" },
	{ "ICMT", "COMMENT" },
	{ "ICOP", "COPYRIGHT" },
	{ "ICRD", "DATE" },
	{ "IENG", "ENGINEER" },
	{ "IGNR", "GENRE" },
	{ "IKEY", "KEYWORDS" },
	{ "IMED", "MEDIUM" },
	{ "INAM", "TITLE" },
	{ "ISFT", "SOFTWARE" },
	{ "ISRC", "SOURCE" },
	{ "ITCH", "TECHNICIAN" },
	{ "ISBJ", "SUBJECT" },
	{ "ISRF", "SOURCE FORM" }
};

static const std::map<std::string /*tag*/, std::string /*type*/> kTagToType = {
	{ "ARTIST", "IART" },
	{ "COMMENT","ICMT" },
	{ "COPYRIGHT", "ICOP" },
	{ "DATE", "ICRD" },
	{ "ENGINEER", "IENG" },
	{ "GENRE", "IGNR" },
	{ "KEYWORDS", "IKEY" },
	{ "MEDIUM", "IMED" },
	{ "TITLE", "INAM" },
	{ "SOFTWARE", "ISFT" },
	{ "SOURCE", "ISRC" },
	{ "TECHNICIAN", "ITCH" },
	{ "SUBJECT", "ISBJ" },
	{ "SOURCE FORM", "ISRF" }
};

struct TagData
{
  using Tag = std::tuple<std::string /*listType*/, std::string /*type*/, std::string /*value*/>;

  TagData( DecoderOpus* decoder )
  {
    for ( const auto& it : kTagToType ) {
      const auto& [ name, type ] = it;
      if ( const auto tag = decoder->GetTagValue( name ); tag.has_value() && !tag->empty() ) {
        m_Tags.push_back( std::make_tuple( "INFO", type, *tag ) );
      }
    }
  }

  std::optional<Tag> GetTag( const uint32_t index )
  {
    if ( index < m_Tags.size() )
      return m_Tags[ index ];
    return std::nullopt;
  }

private:
  std::vector<Tag> m_Tags;
  uint32_t m_CurrentData = 0;
};

struct Input
{
  Input( std::unique_ptr<DecoderOpus> decoder, std::unique_ptr<TagData> tagData ) : m_Decoder( std::move( decoder ) ), m_TagData( std::move( tagData ) ) {};

  DecoderOpus* GetDecoder() { return m_Decoder.get(); }
  TagData* GetTagData() { return m_TagData.get(); }

private:
  std::unique_ptr<DecoderOpus> m_Decoder;
  std::unique_ptr<TagData> m_TagData;
};

int __stdcall QueryCoolFilter( COOLQUERY* cq )
{
  _ASSERT_EXPR( 0, L"QueryCoolFilter" );

	strcpy_s( cq->szName, 24, "Opus" );		
	strcpy_s( cq->szCopyright, 80, EncoderOpus::GetVersion().c_str() );
	std::memcpy( cq->szExt, "OPUS", 4 );
	cq->lChunkSize = 0; 
	cq->dwFlags = QF_CANLOAD | QF_CANSAVE | QF_RATEADJUSTABLE | QF_HASOPTIONSBOX | QF_CANDO32BITFLOATS | QF_READSPECIALFIRST | QF_WRITESPECIALFIRST;
 	cq->Stereo16 = 0xFF;
 	cq->Stereo32 = 0xFF;
 	cq->Mono16 = 0xFF;
 	cq->Mono32 = 0xFF;
  return C_VALIDLIBRARY;
}

BOOL __stdcall FilterUnderstandsFormat( LPSTR filename )
{
  try {
    DecoderOpus decoder( AnsiCodePageToUTF8( filename ) );
    return TRUE;
  } catch ( const std::runtime_error& ) {
    return FALSE;
  }
}

HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND, LONG* chunkSize )
{
  try {
    auto decoder = std::make_unique<DecoderOpus>( AnsiCodePageToUTF8( filename ) );
    if ( nullptr != sampleRate )
      *sampleRate = static_cast<LONG>( decoder->GetSampleRate() );
    if ( nullptr != bitsPerSample )
      *bitsPerSample = static_cast<WORD>( decoder->GetBitsPerSample() );
    if ( nullptr != channels )
      *channels = static_cast<WORD>( decoder->GetChannels() );
    if ( nullptr != chunkSize )
      *chunkSize = static_cast<LONG>( kOpusChunkSize );
    auto tagData = std::make_unique<TagData>( decoder.get() );
    return new Input( std::move( decoder ), std::move( tagData ) );
  } catch ( const std::runtime_error& ) {
    return 0;
  }
}

DWORD __stdcall FilterGetFileSize( HANDLE hInput )
{
  Input* input = static_cast<Input*>( hInput );
  DecoderOpus* decoder = ( nullptr != input ) ? input->GetDecoder() : nullptr;
  if ( nullptr == decoder )
    return 0;

  const uint64_t totalBytes = decoder->GetTotalSamples() * decoder->GetChannels() * decoder->GetBitsPerSample() / 8;
  return static_cast<DWORD>( std::min( totalBytes, static_cast<uint64_t>( std::numeric_limits<DWORD>::max() ) ) );
}

DWORD __stdcall ReadFilterInput( HANDLE hInput, BYTE* data, LONG bytes )
{
  Input* input = static_cast<Input*>( hInput );
  DecoderOpus* decoder = ( nullptr != input ) ? input->GetDecoder() : nullptr;
  if ( nullptr == decoder )
    return 0;
  return decoder->Read( data, bytes );
}

void __stdcall CloseFilterInput( HANDLE hInput )
{
  Input* input = static_cast<Input*>( hInput );
  if ( nullptr != input )
    delete input;
}

void __stdcall GetSuggestedSampleType( LONG* sampleRate, WORD* bitsPerSample, WORD* channels )
{
  *sampleRate = 0;
  *bitsPerSample = 0;
  *channels = 0;
}

HANDLE __stdcall OpenFilterOutput( LPSTR filename, LONG sampleRate, WORD bitsPerSample, WORD channels, LONG size, LONG* chunkSize, DWORD options )
{
  // Ensure the filename ends with a single '.opus' (Cool Edit sometimes repeats itself, probably due to the 4 character extension).
  std::string utf8Filename = AnsiCodePageToUTF8( filename );
  while ( utf8Filename.ends_with( ".opus" ) || utf8Filename.ends_with( ".OPUS" ) ) {
    utf8Filename.resize( utf8Filename.size() - 5 );
  }
  utf8Filename += ".opus";
  const uint32_t bitrate = static_cast<uint32_t>( std::clamp( ReadSetting( kBitrateSetting ).value_or( kOpusDefaultBitrate ), static_cast<int32_t>( kOpusMinimumBitrate ), static_cast<int32_t>( kOpusMaximumBitrate ) ) );
  EncoderOpus* encoder = new EncoderOpus( utf8Filename, static_cast<uint32_t>( sampleRate ), static_cast<uint32_t>( bitsPerSample ), static_cast<uint32_t>( channels ), static_cast<uint32_t>( size / channels / ( bitsPerSample / 8 ) ), bitrate );
  *chunkSize = static_cast<LONG>( kOpusChunkSize );
  return encoder;
}

void __stdcall CloseFilterOutput( HANDLE output )
{
  EncoderOpus* encoder = static_cast<EncoderOpus*>( output );
  if ( nullptr != encoder )
    delete encoder;
}

DWORD __stdcall WriteFilterOutput( HANDLE output, BYTE* data, LONG bytes)
{
  EncoderOpus* encoder = static_cast<EncoderOpus*>( output );
  if ( nullptr == encoder )
    return 0;
  return encoder->Write( data, bytes );
}

INT_PTR CALLBACK DialogProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message ) {
		case WM_INITDIALOG : {
      const int32_t bitrate = std::clamp( ReadSetting( kBitrateSetting ).value_or( kOpusDefaultBitrate ), static_cast<int32_t>( kOpusMinimumBitrate ), static_cast<int32_t>( kOpusMaximumBitrate ) );
      const auto str = std::to_wstring( bitrate );
      SetDlgItemText( hwnd, IDC_BITRATE, str.c_str() );
      return TRUE;
		}
    case WM_COMMAND : {
			switch ( LOWORD( wParam ) ) { 
				case IDOK : {
          try {
						uint32_t result = 0;
            std::vector<wchar_t> buffer( 16 );
            if ( GetDlgItemText( hwnd, IDC_BITRATE, buffer.data(), buffer.size() ) )
              result = std::clamp( static_cast<uint32_t>( std::stoul( buffer.data() ) ), kOpusMinimumBitrate, kOpusMaximumBitrate );
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
  Input* input = static_cast<Input*>( hInput );
  DecoderOpus* decoder = ( nullptr != input ) ? input->GetDecoder() : nullptr;
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
  EncoderOpus* encoder = static_cast<EncoderOpus*>( output );
  if ( nullptr != encoder && nullptr != listType && nullptr != type && nullptr != data ) {
    if ( std::string( "INFO" ) == listType ) {
      if ( const auto tag = kTypeToTag.find( type ); kTypeToTag.end() != tag )
        encoder->AddTag( tag->second, AnsiCodePageToUTF8( std::string( data, size ) ) );
    }
  }
  return 1;
}

DWORD GetNextTag( SPECIALDATA* specialData, TagData* tagData )
{
  if ( nullptr == specialData || nullptr == tagData )
    return 0;

  uint32_t index = reinterpret_cast<uint32_t>( specialData->hSpecialData );
  const auto tag = tagData->GetTag( index );
  if ( !tag )
    return 0;

  const auto& [ listType, type, value ] = *tag;

  // Following the Flt2KApi example, allocate some space on the heap for the tag data (it is not our responsibility to free this handle).
  specialData->hData = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 2 + value.size() );
  if ( nullptr == specialData->hData )
    return 0;

  if ( char* str = static_cast<char*>( GlobalLock( specialData->hData ) ); nullptr != str ) {
    strcpy_s( str, 1 + value.size(), value.c_str() );
    GlobalUnlock( str );
  }
  specialData->dwExtra = 1;
  specialData->dwSize = 1 + value.size();
  strcpy_s( specialData->szListType, 8, listType.c_str() );
  strcpy_s( specialData->szType, 8, type.c_str() );

  specialData->hSpecialData = reinterpret_cast<HANDLE>( ++index );
  return 1;
}

DWORD __stdcall FilterGetFirstSpecialData( HANDLE hInput, SPECIALDATA* specialData )
{
  Input* input = static_cast<Input*>( hInput );
  TagData* tagData = ( nullptr != input ) ? input->GetTagData() : nullptr;
  if ( nullptr == tagData || nullptr == specialData )
    return 0;

  specialData->hSpecialData = 0;
  return GetNextTag( specialData, tagData );
}

DWORD __stdcall FilterGetNextSpecialData( HANDLE hInput, SPECIALDATA* specialData )
{
  Input* input = static_cast<Input*>( hInput );
  TagData* tagData = ( nullptr != input ) ? input->GetTagData() : nullptr;
  return GetNextTag( specialData, tagData );
}
