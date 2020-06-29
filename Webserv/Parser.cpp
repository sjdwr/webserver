#include "Parser.h"

BOOLEAN
AnalyzeHeader(
	LPCSTR Hdr,
	MUST_FREE_MEMORY pHdr pHeader,	// pHeader->RequestHeader, pHeader->RequestURLQuery
	LPSTR pHeaderEnd,
	DWORD ContentRead
)
{
	BOOLEAN HdrFound = FALSE;
	LPCSTR szmethods[] = { "GET", "POST", "HEAD", "PUT", "PATCH", "DELETE", "OPTIONS", "TRACE" };
	LPSTR parsIndex = (LPSTR)Hdr;
	LPSTR pTmp;
	LPSTR pContext;
	DWORD TypeLen;
	DWORD i, j;

	if (NULL == pHeader ||
		NULL == Hdr)
		return FALSE;

	pHeader->HeaderEnd = pHeaderEnd;
	pHeader->ContentRead = ContentRead;

	for (i = 0; i < 8; ++i)
	{
		TypeLen = strlen(szmethods[i]);

		if (0 == strncmp(Hdr, szmethods[i], TypeLen))
		{
			pHeader->HdrMethods = (methods)i;
			HdrFound = TRUE;
			break;
		}
	}

	if (FALSE == HdrFound)
		return FALSE;

	parsIndex += TypeLen + 1;

	++parsIndex; // Skip(/)
	pHeader->RequestURL = parsIndex;  // + 1= Skip(/)

	pTmp = strstr(parsIndex, " HTTP/");

	if (NULL == pTmp)
		return FALSE;

	*pTmp = NULL;

	pTmp += 6;
	pHeader->HTTPVersion = pTmp;

	pTmp += 3;
	SKIP_RETURNCHAR(pTmp);

	parsIndex = pTmp;

	if (pTmp = strstr(pHeader->RequestURL, "?")) // URL 에 변수가 있을 시 
	{
		*pTmp++ = NULL;
		AnalyzeQuery(pTmp, pHeader->RequestURLQuery, pHeader->RequestURLQueryCount);
	}

	j = 10;
	pHeader->RequestHeader = (pReqHdr)malloc(sizeof(ReqHdr) * j);

	if (NULL == pHeader->RequestHeader)
		return FALSE;

	pTmp = strtok_s(parsIndex, "\r\n", &pContext);
	i = 0;

	while (pTmp)
	{
		LPSTR pTok;

		if (i >= j)
		{
			j += 10;
			pHeader->RequestHeader = (pReqHdr)realloc(pHeader->RequestHeader, sizeof(ReqHdr) * j);

			if (NULL == pHeader->RequestHeader)
				return FALSE;
		}

		pHeader->RequestHeader[i].Alloc = 0;
		pHeader->RequestHeader[i].Name = pTmp;
		pTok = strchr(pTmp, ':');

		if (NULL == pTok)
			break;

		*pTok++ = NULL;

		SKIP_SPACECHAR(pTok);
		pHeader->RequestHeader[i].Value = pTok;

		++i;
		pTmp = strtok_s(NULL, "\r\n", &pContext);
	}

	pHeader->RequestHeaderCount = i;

	return TRUE;
}

BOOLEAN
ParseHeader(
	LPSTR		Hdr,
	pReqHdr		pHeader,
	DWORD		&HeaderCount
)
{
	LPSTR		pContext;
	LPSTR		pTok;
	LPSTR		pTmp;
	DWORD		Cnt = 0;

	HeaderCount = 0;
	pTmp = strtok_s(Hdr, "\r\n", &pContext);

	while (pTmp)
	{
		pHeader[Cnt].Alloc = 0;
		pHeader[Cnt].Name = pTmp;
		pTok = strchr(pTmp, ':');

		if (NULL == pTok)
			return FALSE;

		*pTok++ = NULL;

		SKIP_SPACECHAR(pTok);
		pHeader[Cnt].Value = pTok;
		++Cnt;

		pTmp = strtok_s(NULL, "\r\n", &pContext);
	}

	HeaderCount = Cnt;

	return TRUE;
}

BOOLEAN 
ParseRange(
	pHdr pHeader, 
	QWORD &StartPos, 
	QWORD &EndPos
)
{
	LPSTR pEndPtr = NULL;

	if (NULL == pHeader)
		return FALSE;

	for (DWORD Index = 0; Index < pHeader->RequestHeaderCount; ++Index)
	{
		if (0 == strncmp(pHeader->RequestHeader[Index].Name, "Range", 5) &&
			0 == strncmp(pHeader->RequestHeader[Index].Value, "bytes=", 6))
		{
			LPSTR Range = pHeader->RequestHeader[Index].Value + 6;
			LPSTR pRangeTok = strchr(Range, '-');

			if (NULL == pRangeTok)
				pRangeTok = Range + strlen(Range);

			*pRangeTok = NULL;
			StartPos = strtoull(Range, &pEndPtr, 10);
			EndPos = *(pRangeTok + 1) ? strtoull(pRangeTok + 1, &pEndPtr, 10) : 0;

			return TRUE;
		}
	}

	return FALSE;
}

LPSTR 
FindHeaderValue(
	pHdr pHeader, 
	LPCSTR szName
)
{
	if (NULL == pHeader)
		return NULL;

	for (DWORD i = 0; i < pHeader->RequestHeaderCount; ++i)
	{
		if (0 == strncmp(pHeader->RequestHeader[i].Name, szName, strlen(szName)))
			return pHeader->RequestHeader[i].Value;
	}

	return NULL;
}

LPSTR
FindVarNameValue(
	pReqVar _pReqVar,
	DWORD VarCount, 
	LPCSTR szName
)
{
	if (NULL == _pReqVar)
		return NULL;

	for (DWORD Index = 0; Index < VarCount; ++Index)
	{
		if (0 == strncmp(_pReqVar[Index].Name, szName, strlen(szName)))
			return _pReqVar[Index].Value;
	}

	return NULL;
}

BOOLEAN
AnalyzeQuery(
	LPSTR Str,
	MUST_FREE_MEMORY pReqVar &_pReqVar,
	DWORD &VarCount
)
{
	DWORD j = 5;
	DWORD i = 0;

	LPSTR delimiter = NULL;
	LPSTR pContext;
	LPSTR pTok;

	if (NULL == Str)
		return FALSE;

	_pReqVar = (pReqVar)malloc(sizeof(ReqVar) * j);

	if (NULL == _pReqVar)
		return FALSE;

	delimiter = strtok_s(Str, "&", &pContext);

	while (delimiter)
	{
		if (i >= j)
		{
			j += 5;
			_pReqVar = (pReqVar)realloc(_pReqVar, sizeof(ReqVar) * j);

			if (NULL == _pReqVar)
				return FALSE;
		}

		if ('=' == *delimiter)
			goto RELEASE;

		pTok = strchr(delimiter, '=');

		if (NULL == pTok)
			goto RELEASE;

		_pReqVar[i].Name = delimiter;
		*pTok++ = NULL;
		_pReqVar[i].Value = pTok;

		++i;
		delimiter = strtok_s(NULL, "&", &pContext);
	}

	VarCount = i;

	return TRUE;

RELEASE:

	free(_pReqVar);
	_pReqVar = NULL;

	return FALSE;
}

BOOLEAN
AnalyzeCookie(
	LPSTR Str,
	MUST_FREE_MEMORY pReqVar &_pReqVar,
	DWORD &VarCount
)
{
	DWORD j = 5;
	DWORD i = 0;

	LPSTR delimiter = NULL;
	LPSTR pContext;
	LPSTR pTok;

	if (NULL == Str)
		return FALSE;

	_pReqVar = (pReqVar)malloc(sizeof(ReqVar) * j);

	if (NULL == _pReqVar)
		return FALSE;

	delimiter = strtok_s(Str, ";", &pContext);

	while (delimiter)
	{
		if (i >= j)
		{
			j += 5;
			_pReqVar = (pReqVar)realloc(_pReqVar, sizeof(ReqVar) * j);

			if (NULL == _pReqVar)
				return FALSE;
		}

		SKIP_SPACECHAR(delimiter);

		if ('=' == *delimiter)
			goto RELEASE;

		_pReqVar[i].Name = delimiter;

		if (pTok = strchr(delimiter, '='))
		{
			*pTok++ = NULL;
			_pReqVar[i].Value = pTok;
		}

		++i;
		delimiter = strtok_s(NULL, ";", &pContext);
	}

	VarCount = i;

	return TRUE;

RELEASE:

	free(_pReqVar);
	_pReqVar = NULL;

	return FALSE;
}