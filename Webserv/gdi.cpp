#include "gdi.h"

CLSID	clsid;

ULONG_PTR InitGdi()
{
	ULONG_PTR token;
	GdiplusStartupInput Input = { 0 };

	CoInitialize(NULL);

	Input.GdiplusVersion = 1;
	GdiplusStartup(&token, &Input, NULL);
	CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &clsid);

	return token;
}

VOID ShutdownGdi(ULONG_PTR token)
{
	GdiplusShutdown(token);
	CoUninitialize();
}

BOOLEAN save_jpg_memory(HBITMAP hBitmap, LPSTR &Data, LPDWORD lpdwReturnDataSize)
{
	LPVOID		GpBitmap;
	IStream		*IStream = NULL;
	HGLOBAL		hGbl = NULL;
	SIZE_T		bufsize;
	LPVOID		pJpgBinary;
	GpStatus	Status;
	BOOLEAN		bResult = FALSE;

	GdipCreateBitmapFromHBITMAP(hBitmap, NULL, &GpBitmap);
	CreateStreamOnHGlobal(NULL, TRUE, &IStream);
	Status = GdipSaveImageToStream(GpBitmap, IStream, &clsid, NULL);
	GdipDisposeImage(GpBitmap);

	if (GpStatus::Ok != Status)
		goto RELEASE;

	GetHGlobalFromStream(IStream, &hGbl);
	bufsize = GlobalSize(hGbl);
	pJpgBinary = GlobalLock(hGbl);

	if (NULL == pJpgBinary)
		goto RELEASE;

	Data = (LPSTR)malloc(bufsize);

	if (NULL == Data)
		goto RELEASE;

	CopyMemory(Data, pJpgBinary, bufsize);

	if (lpdwReturnDataSize)
		*lpdwReturnDataSize = bufsize;

	bResult = TRUE;

RELEASE:

	if (hGbl)
	{
		GlobalUnlock(hGbl);
		//GlobalFree(hGbl);
	}

	if (IStream)
		IStream->Release();

	return bResult;
}