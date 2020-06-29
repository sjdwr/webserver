#ifndef HTTP_H
#define HTTP_H

#include "main.h"
#include "unit.h"
#include "union.h"

// FUNCTION
BOOLEAN
HTTP_SendClientHeader(
	SOCKET		sck,
	pHdr		pHeader,
	QWORD		ContentLength
);

BOOLEAN
HTTP_SendClientFile(
	SOCKET		sck,
	HANDLE		hFile,
	QWORD		ContentLength
);

BOOLEAN
HTTP_SendClientText(
	SOCKET		sck,
	LPCSTR		_Format,
	...
);

BOOLEAN
HTTP_SendClientText_Pure(
	SOCKET		sck,
	LPCSTR		Text,
	DWORD		ContentLength
);

BOOLEAN
HTTP_SendFile(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Path
);

BOOLEAN
HTTP_GetContent(
	SOCKET		sck,
	pHdr		pHeader,
	MUST_FREE_MEMORY LPSTR &pContent,
	BOOLEAN		&AllocateContent
);

BOOLEAN
HTTP_AddHeader(
	pHdr	pHeader,
	LPCSTR	HeaderName,
	LPCSTR	HeaderValue
);

VOID
HTTP_FreeHeader(
	pHdr pHeader
);

BOOLEAN
HTTP_ExtUploadProgress(
	PSOCKETEX		ClientSckArray,
	DWORD			MaxClinet,
	pHdr			pHeader,
	QWORD			&TotalRead,
	QWORD			&ContentLength
);

BOOLEAN
HTTP_SetSckResponeSession(
	PSOCKETEX		psckEx,
	pHdr			pHeader
);

BOOLEAN
HTTP_Upload(
	PSOCKETEX	psckEx,
	pHdr		pHeader,
	LPCSTR		UploadPath
);

BOOLEAN
HTTP_ContextFile(
	SOCKET		sck,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		ShUploadAndChat
);

VOID
HTTP_Context404(
	SOCKET		sck,
	pHdr		pHeader
);

VOID
HTTP_ContextUpload(
	SOCKET		sck,
	pHdr		pHeader
);

VOID
HTTP_ContextChat(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		ServerIP
);

#endif