#ifndef UTF_8_H
#define UTF_8_H

#include "union.h"

// FUNCTION
BOOLEAN 
UTF8toANSI(
	LPCSTR utf8,
	MUST_FREE_MEMORY LPSTR &ansi
);

BOOLEAN
UTF8toANSI_NoAlloc(
	LPCSTR utf8,
	LPSTR ansi
);

DWORD
ANSItoUTF8(
	LPCSTR ansi,
	MUST_FREE_MEMORY LPSTR &utf8
);

BOOLEAN
URLDecode(
	LPSTR Src
);

MUST_FREE_MEMORY LPSTR
URLEncode(
	LPCSTR Src
);

#endif