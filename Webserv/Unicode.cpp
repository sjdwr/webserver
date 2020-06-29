#include "Unicode.h"

BOOLEAN 
UTF8toANSI(
	LPCSTR utf8, 
	MUST_FREE_MEMORY LPSTR &ansi
)
{
	// convert multibyte UTF-8 to wide string UTF-16
	INT length = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, NULL, 0);

	if (length > 0)
	{
		LPWSTR lpwsz = (LPWSTR)malloc((length + 1) * sizeof(WCHAR));

		if (NULL == lpwsz)
			return FALSE;

		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, lpwsz, length);

		length = WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, NULL, 0, NULL, NULL);

		LPSTR lpszbuf = (LPSTR)malloc(length + 1);

		if (NULL == lpszbuf)
		{
			free(lpwsz);
			return FALSE;
		}

		if (length)
			WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, lpszbuf, length, 0, 0);

		ansi = lpszbuf;

		free(lpwsz);
	}

	return TRUE;
}

BOOLEAN
UTF8toANSI_NoAlloc(
	LPCSTR utf8,
	LPSTR ansi
)
{
	// convert multibyte UTF-8 to wide string UTF-16
	INT length = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, NULL, 0);

	if (length > 0)
	{
		LPWSTR lpwsz = (LPWSTR)malloc((length + 1) * sizeof(WCHAR));

		if (NULL == lpwsz)
			return FALSE;

		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, lpwsz, length);

		length = WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, NULL, 0, NULL, NULL);

		if (length)
			WideCharToMultiByte(CP_ACP, 0, lpwsz, -1, ansi, length, 0, 0);

		free(lpwsz);

		ansi[length] = NULL;
	}

	return TRUE;
}

DWORD 
ANSItoUTF8(
	LPCSTR ansi, 
	MUST_FREE_MEMORY LPSTR &utf8
)
{
	INT length = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);

	if (length > 0)
	{
		LPWSTR lpwsz = (LPWSTR)malloc(sizeof(WCHAR) * (length + 1));

		if (NULL == lpwsz)
			return FALSE;

		MultiByteToWideChar(CP_ACP, 0, ansi, -1, lpwsz, length);

		length = WideCharToMultiByte(CP_UTF8, 0, lpwsz, -1, NULL, 0, NULL, NULL);

		LPSTR lpszbuf = (LPSTR)malloc(sizeof(CHAR) * (length + 1));

		if (NULL == lpszbuf)
		{
			free(lpwsz);
			return FALSE;
		}

		if (length)
			WideCharToMultiByte(CP_UTF8, 0, lpwsz, -1, lpszbuf, length, 0, 0);

		utf8 = lpszbuf;
		free(lpwsz);

		utf8[length] = NULL;
	}

	return length;
}

CHAR
HEXTOCHAR(
	LPSTR Hex
)
{
	CHAR c;

	if (Hex[0] >= 'A' && Hex[0] <= 'F')
		c = (Hex[0] - 'A' + 10) * 16;
	else if (Hex[0] >= 'a' && Hex[0] <= 'f')
		c = (Hex[0] - 'a' + 10) * 16;
	else
		c = (Hex[0] - '0') * 16;

	if (Hex[1] >= 'A' && Hex[1] <= 'F')
		c += (Hex[1] - 'A' + 10);
	else if (Hex[1] >= 'a' && Hex[1] <= 'f')
		c += (Hex[1] - 'a' + 10);
	else
		c += (Hex[1] - '0');

	return c;
}

BOOLEAN
URLDecode(
	LPSTR Src
)
{
	UINT		Len;
	UINT		j = 0;
	UINT		k = 0;

	LPWSTR		lpwsz = NULL;
	LPSTR		lpszbuf = NULL;

	if (NULL == Src)
		return FALSE;

	Len = strlen(Src);

	while (*(Src + k))
	{
		switch (*(Src + k))
		{
			case '%':
			{
				if (k + 2 >= Len)
					return FALSE;

				*(Src + j) = HEXTOCHAR(Src + k + 1);

				k += 2;

				break;
			}
			case '+':
			{
				*(Src + j) = ' ';
				break;
			}
			default:
				*(Src + j) = *(Src + k);
		}

		++j;
		++k;
	}

	*(Src + j) = NULL;

	return UTF8toANSI_NoAlloc(Src, Src);
}

MUST_FREE_MEMORY LPSTR
URLEncode(
	LPCSTR Src
)
{
	LPSTR		covStr = NULL;
	LPSTR		utf8 = NULL;
	size_t		len;
	UINT		i = 0, j = 0;

	if (NULL == Src)
		return NULL;

	if (FALSE == ANSItoUTF8(Src, utf8))
		return NULL;

	len = strlen(utf8);
	
	if (NULL == (covStr = (LPSTR)malloc(sizeof(CHAR) * len * 3 + 1)))
		goto RELEASE;

	while (utf8[i])
	{
		if ( utf8[i] == '\\' ||
			 utf8[i] == '/' ||
			 utf8[i] == '.' ||
			 utf8[i] == '@' ||
			 utf8[i] == '-' ||
			 utf8[i] == '_' ||
			 utf8[i] == ':' ||
			 (utf8[i] >= 'a' && utf8[i] <= 'z') ||
			 (utf8[i] >= 'A' && utf8[i] <= 'Z') ||
			 (utf8[i] >= '0' && utf8[i] <= '9') )
		{
			covStr[j++] = utf8[i];
		}
		else
		{
			sprintf_s(covStr + j, len * 3 + 1 - j, "%%%02x", (BYTE)utf8[i]);
			j += 3;
		}

		++i;
	}

	covStr[j] = NULL;

RELEASE:

	if (utf8)
		free(utf8);

	return covStr;
}