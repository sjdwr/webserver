#ifndef GDI_H
#define GDI_H

#include <Windows.h>

#pragma comment(lib, "gdiplus.lib")

enum GpStatus
{
	Ok = 0,
	GenericError = 1,
	InvalidParameter = 2,
	OutOfMemory = 3,
	ObjectBusy = 4,
	InsufficientBuffer = 5,
	NotImplemented = 6,
	Win32Error = 7,
	WrongState = 8,
	Aborted = 9,
	FileNotFound = 10,
	ValueOverflow = 11,
	AccessDenied = 12,
	UnknownImageFormat = 13,
	FontFamilyNotFound = 14,
	FontStyleNotFound = 15,
	NotTrueTypeFont = 16,
	UnsupportedGdiplusVersion = 17,
	GdiplusNotInitialized = 18,
	PropertyNotFound = 19,
	PropertyNotSupported = 20,
#if (GDIPVER >= 0x0110)
	ProfileNotFound = 21,
#endif
};

struct GdiplusStartupInput
{
	UINT32 GdiplusVersion;             // Must be 1  (or 2 for the Ex version)
	LPVOID DebugEventCallback;		   // Ignored on free builds
	BOOL SuppressBackgroundThread;     // FALSE unless you're prepared to call the hook/unhook functions properly
	BOOL SuppressExternalCodecs;       // FALSE unless you want GDI+ only to use its internal image codecs.
};

/*
	image/bmp  : {557cf400-1a04-11d3-9a73-0000f81ef32e}
	image/jpeg : {557cf401-1a04-11d3-9a73-0000f81ef32e}
	image/gif  : {557cf402-1a04-11d3-9a73-0000f81ef32e}
	image/tiff : {557cf405-1a04-11d3-9a73-0000f81ef32e}
	image/png  : {557cf406-1a04-11d3-9a73-0000f81ef32e}
*/

extern "C"
{
	GpStatus WINAPI
		GdipSaveImageToStream(
			LPVOID image,
			IStream* stream,
			LPCVOID clsidEncoder,
			LPCVOID encoderParams
		);

	GpStatus WINAPI
		GdipCreateBitmapFromHBITMAP(
			HBITMAP hbm,
			HPALETTE hpal,
			LPVOID *bitmap
		);

	GpStatus WINAPI
		GdipDisposeImage(
			LPVOID image
		);

	GpStatus WINAPI
		GdiplusStartup(
			OUT ULONG_PTR *token,
			const GdiplusStartupInput *input,
			LPVOID output);


	VOID WINAPI
		GdiplusShutdown(
			ULONG_PTR token
		);
}

ULONG_PTR		
InitGdi(
);

VOID
ShutdownGdi(
	ULONG_PTR	token
);

BOOLEAN	save_jpg_memory(
	HBITMAP		hBitmap, 
	LPSTR		&Data, 
	LPDWORD		lpdwReturnDataSize
);

#endif