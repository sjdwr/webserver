#ifndef PARSER_H
#define PARSER_H

#include "union.h"

// FUNCTION
BOOLEAN
AnalyzeHeader(
	LPCSTR Hdr,
	MUST_FREE_MEMORY pHdr pHeader,	// pHeader->RequestHeader
	LPSTR pHeaderEnd,
	DWORD ContentRead
);

BOOLEAN
ParseRange(
	pHdr		pHeader,
	QWORD		&StartPos,
	QWORD		&EndPos
);

BOOLEAN
ParseHeader(
	LPSTR		Hdr,
	pReqHdr		pHeader,
	DWORD		&HeaderCount
);

LPSTR
FindHeaderValue(
	pHdr		pHeader,
	LPCSTR		szName
);

LPSTR
FindVarNameValue(
	pReqVar		_pReqVar,
	DWORD		VarCount,
	LPCSTR		szName
);

BOOLEAN
AnalyzeQuery(
	LPSTR		Str,
	MUST_FREE_MEMORY pReqVar &_pReqVar,
	DWORD		&VarCount
);

BOOLEAN
AnalyzeCookie(
	LPSTR Str,
	MUST_FREE_MEMORY pReqVar &_pReqVar,
	DWORD &VarCount
);
#endif