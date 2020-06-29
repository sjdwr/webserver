#ifndef UNIT_H
#define UNIT_H

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <Shlobj.h>

VOID
SizeToUnit(
	LPSTR dst,
	size_t dstlen,
	DWORD High,
	DWORD Low
);

VOID
MakeSession(
	LPSTR Session,
	DWORD RandomSize
);

DWORD
Replace(
	LPSTR &Dst,
	LPSTR Src,
	LPCSTR find,
	LPCSTR replace
);

DWORD
CovTag(
	LPSTR &Dst
);

DWORD
StrCount(
	LPCSTR Str,
	LPCSTR Substr
);

VOID
MakeRandomString(
	LPSTR Session,
	DWORD RandomSize
);

BOOLEAN
BrowserFolder(
	HWND		hWndOwner,
	LPCSTR		DialogTitle,
	LPSTR		ResultPath,
	size_t		PathLen
);

VOID
RemoveSpecialChar(
	LPSTR szName,
	size_t len
);

#endif