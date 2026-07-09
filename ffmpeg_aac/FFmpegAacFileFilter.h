#pragma once

#include "filters.h"

#ifdef __cplusplus
extern "C" {
#endif
int __stdcall QueryCoolFilter( COOLQUERY* cq );
BOOL __stdcall FilterUnderstandsFormat( LPSTR filename );
HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND hwnd, LONG* chunkSize );
DWORD __stdcall FilterGetFileSize( HANDLE input );
DWORD __stdcall ReadFilterInput( HANDLE input, BYTE* data, LONG bytes );
void __stdcall CloseFilterInput( HANDLE input );
void __stdcall GetSuggestedSampleType( LONG* sampleRate, WORD* bitsPerSample, WORD* channels );
HANDLE __stdcall OpenFilterOutput( LPSTR filename, LONG sampleRate, WORD bitsPerSample, WORD channels, LONG size, LONG* chunkSize, DWORD options );
void __stdcall CloseFilterOutput( HANDLE output );
DWORD __stdcall WriteFilterOutput( HANDLE output, BYTE* data, LONG bytes);
DWORD __stdcall FilterGetOptions( HWND hwnd, HINSTANCE inst, LONG sampleRate, WORD channels, WORD bitsPerSample, DWORD options );
DWORD __stdcall FilterOptions( HANDLE input );
DWORD __stdcall FilterOptionsString( HANDLE input, LPSTR str );
DWORD __stdcall FilterWriteSpecialData( HANDLE output, LPCSTR listType, LPCSTR type, char* data, DWORD size );
DWORD __stdcall FilterGetFirstSpecialData( HANDLE input, SPECIALDATA* specialData );
DWORD __stdcall FilterGetNextSpecialData( HANDLE input, SPECIALDATA* specialData );
#ifdef __cplusplus
}
#endif
