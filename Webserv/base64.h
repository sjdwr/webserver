#ifndef BASE64_H
#define BASE64_H

#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#define __base64_encode_len(len) ((len + 3 - len % 3) * 4 / 3 + 1)

VOID 
__base64_encode(
	LPSTR Dst, 
	LPCSTR Src, 
	DWORD length
);

VOID 
__base64_decode(
	LPSTR Dst, 
	LPCSTR Src, 
	INT length
);

#endif
