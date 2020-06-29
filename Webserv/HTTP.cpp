#include "HTTP.h"

MIME_TYPE SUPPORT_MIMES[] =
{
	{"png",		"image/png"			},
	{"jpeg",	"image/jpeg"		},
	{"jpg",		"image/jpeg"		},
	{"gif",		"image/gif"			},
	{"css",		"text/css"			},
	{"js",		"text/javascript"	},
	{"html",	"text/html"			},
	{"mp3",		"audio/mpeg"		},
	{"mp4",		"video/mp4"			},
	{"ts",		"video/MP2T"		},
	{"mkv",		"video/x-matroska"	}
};

BOOLEAN 
HTTP_SendClientHeader(
	SOCKET	sck, 
	pHdr	pHeader,
	QWORD	ContentLength
)
{
	CHAR		Hdr[1024];
	LPSTR		pTmp;
	LONG		bytesTransferred;

	DWORD		HeaderLength = 0;
	DWORD		TotalTransferred;
	DWORD		MIME_INDEX;
	QWORD		StartRange = 0;
	QWORD		EndRange = 0;

	BOOLEAN		Extension = FALSE;
	BOOLEAN		UsageRange;

	if (0 == ContentLength)
		return FALSE;

	if (NULL == pHeader)
		return FALSE;

	pTmp = pHeader->RequestURL;
	GETFILEEXT(pTmp);
	
	if (pTmp)
	{
		for (MIME_INDEX = 0; MIME_INDEX < sizeof(SUPPORT_MIMES) / sizeof(MIME_TYPE); ++MIME_INDEX)
		{
			if (0 == strncmp(pTmp, SUPPORT_MIMES[MIME_INDEX].Extension, strlen(SUPPORT_MIMES[MIME_INDEX].Extension)))
			{
				Extension = TRUE;
				break;
			}
		}
	}

	UsageRange = ParseRange(pHeader, StartRange, EndRange);

	HeaderLength = sprintf_s(Hdr, sizeof(Hdr), 
		"HTTP/1.1 %s\n", UsageRange ? REQUEST_HEADER_206 : REQUEST_HEADER_200);
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Server: %s\n", SERVER_NAME);
	HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
		"Content-Type: %s\n", Extension ? SUPPORT_MIMES[MIME_INDEX].Mime : "*/*");

	if (pHeader->SendHeaderCount > 0 && pHeader->SendHeader)
	{
		for (UINT Index = 0; Index < pHeader->SendHeaderCount; ++Index)
		{
			HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
				"%s: %s\n", 
				pHeader->SendHeader[Index].Name, 
				pHeader->SendHeader[Index].Value);
		}
	}

	if (0 == EndRange)
		EndRange = ContentLength;

	if (UsageRange)
	{
		pHeader->Download = FALSE;

		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Length: %llu\n", EndRange - StartRange);
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Range: bytes %llu-%llu/%llu\n", StartRange, EndRange - 1, ContentLength);
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Accept-Ranges: bytes\nConnection: close\n");
	}
	else
	{
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Length: %llu\n", ContentLength);
	}

	if (pHeader->Download)
	{
		GETFILENAME(pHeader->RequestURL);
		HeaderLength += sprintf_s(Hdr + HeaderLength, sizeof(Hdr) - HeaderLength,
			"Content-Disposition: attachment; filename=\"%s\";\n", pHeader->RequestURL);
	}

	Hdr[HeaderLength++] = '\n';
	Hdr[HeaderLength] = NULL;

	bytesTransferred = 0;
	TotalTransferred = 0;

	while (TotalTransferred < HeaderLength)
	{
		if (0 >= (bytesTransferred = send(sck, Hdr + TotalTransferred, HeaderLength - TotalTransferred, 0)))
			return FALSE;

		TotalTransferred += bytesTransferred;
	}

	return TRUE;
}

BOOLEAN
HTTP_SendClientFile(
	SOCKET		sck,
	HANDLE		hFile,
	QWORD		ContentLength
)
{
	DWORD		AllocSize;
	QWORD		totalToSend;
	DWORD		amountRead = 0;
	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	LPSTR		Buffer;
	BOOLEAN		bResult = FALSE;

	if (0 == ContentLength)
		return FALSE;

	AllocSize = (DWORD)(ContentLength > MAX_FILEBUF ? MAX_FILEBUF : ContentLength);

	if (NULL == (Buffer = (LPSTR)malloc(sizeof(CHAR) * AllocSize)))
		return FALSE;

	totalToSend = 0;

	do
	{
		if (NULL == ReadFile(hFile, Buffer, AllocSize, &amountRead, NULL) || 0 == amountRead)
			goto RELEASE;

		bytesTransferred = 0;
		TotalTransferred = 0;

		while (TotalTransferred < amountRead)
		{
			if (0 >= (bytesTransferred = send(sck, Buffer + TotalTransferred, amountRead - TotalTransferred, 0)))
				goto RELEASE;

			TotalTransferred += bytesTransferred;
		}

		totalToSend += TotalTransferred;

	} while (ContentLength > totalToSend);

	bResult = TRUE;

RELEASE:

	free(Buffer);

	return bResult;
}

BOOLEAN
HTTP_SendClientText(
	SOCKET		sck,
	LPCSTR		_Format,
	...
)
{

	DWORD		ContentLength = 0;
	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	LPSTR		Buffer = NULL;
	va_list		arg_ptr = NULL;
	BOOLEAN		bResult = FALSE;

	if (NULL == _Format)
		return FALSE;

	va_start(arg_ptr, _Format);

	if (0 == (ContentLength = _vscprintf(_Format, arg_ptr)))
		goto RELEASE;

	++ContentLength;

	if (NULL == (Buffer = (LPSTR)malloc(ContentLength * sizeof(CHAR))))
		goto RELEASE;

	vsprintf_s(Buffer, ContentLength, _Format, arg_ptr);

	bytesTransferred = 0;
	TotalTransferred = 0;

	do
	{
		if (SOCKET_ERROR == (bytesTransferred = send(sck, Buffer + TotalTransferred, ContentLength - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;

	} while (ContentLength > TotalTransferred);


	bResult = TRUE;

RELEASE:

	if (Buffer)
		free(Buffer);

	if (arg_ptr)
		va_end(arg_ptr);

	return bResult;
}

BOOLEAN
HTTP_SendClientText_Pure(
	SOCKET		sck,
	LPCSTR		Text,
	DWORD		ContentLength
)
{

	DWORD		TotalTransferred = 0;
	LONG		bytesTransferred = 0;
	BOOLEAN		bResult = FALSE;

	bytesTransferred = 0;
	TotalTransferred = 0;

	do
	{
		if (SOCKET_ERROR == (bytesTransferred = send(sck, Text + TotalTransferred, ContentLength - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;

	} while (ContentLength > TotalTransferred);


	bResult = TRUE;

RELEASE:

	return bResult;
}

BOOLEAN 
HTTP_SendFile(
	SOCKET sck,
	pHdr pHeader,
	LPCSTR Path
)
{
	QWORD			ContentLength;
	QWORD			RangeStart = 0;
	QWORD			RangeEnd = 0;
	HANDLE			hFile;
	CHAR			PATH[MAX_PATH];
	DWORD			dwSizeHigh;
	LARGE_INTEGER	li;

	if (NULL == pHeader)
		return FALSE;

	sprintf_s(PATH, sizeof(PATH), "%s\\%s", Path, pHeader->RequestURL);

	hFile = CreateFileA(
		PATH,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (INVALID_HANDLE_VALUE == hFile)
		return FALSE;
	
	ContentLength = GetFileSize(hFile, &dwSizeHigh);

	if (dwSizeHigh > 0)
		ContentLength |= dwSizeHigh << 28;

	pHeader->Download = TRUE;
	HTTP_SendClientHeader(sck, pHeader, ContentLength);

	if (_HEAD != pHeader->HdrMethods)
	{
		if (ParseRange(pHeader, RangeStart, RangeEnd))
		{
			li.QuadPart = RangeStart;
			SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
		}

		HTTP_SendClientFile(sck, hFile, ContentLength);
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOLEAN
HTTP_GetContent(
	SOCKET		sck,
	pHdr		pHeader,
	MUST_FREE_MEMORY LPSTR &pContent,
	BOOLEAN		&AllocateContent
)
{
	LPCSTR		pLength;
	LPSTR		pEndHdr;
	DWORD		ContentLength;
	DWORD		ContentRead;

	if (NULL == pHeader ||
		NULL == (pEndHdr = pHeader->HeaderEnd))
		return FALSE;

	ContentRead = pHeader->ContentRead;

	if (pLength = FindHeaderValue(pHeader, "Content-Length"))
		ContentLength = atoi(pLength);
	else
		return FALSE;

	if (ContentLength > ContentRead)
	{
		if (FALSE == ReadSocketContent_Text(sck, pContent, ContentLength, ContentRead))
			return FALSE;

		memcpy(pContent, pEndHdr, ContentRead);
		AllocateContent = TRUE;
	}
	else
	{
		pContent = pEndHdr;
		AllocateContent = FALSE;
	}

	return TRUE;
}

BOOLEAN
HTTP_AddHeader(
	pHdr	pHeader,
	LPCSTR	HeaderName,
	LPCSTR	HeaderValue
)
{
	DWORD		SndHdrIndex;
	DWORD		HeaderNameLen;
	DWORD		HeaderValueLen;

	LPSTR		NewHeaderName = NULL;
	LPSTR		NewHeaderValue = NULL;
	pReqHdr		NewHeader = NULL;

	if (NULL == pHeader ||
		NULL == HeaderName ||
		NULL == HeaderValue)
		return FALSE;
	
	HeaderNameLen = strlen(HeaderName);
	HeaderValueLen = strlen(HeaderValue);

	if (0 == HeaderNameLen ||
		0 == HeaderValueLen)
		return FALSE;

	SndHdrIndex = pHeader->SendHeaderCount;

	if (0 == SndHdrIndex)
		NewHeader = (pReqHdr)malloc(sizeof(ReqHdr));
	else
		NewHeader = (pReqHdr)realloc(pHeader->SendHeader, sizeof(ReqHdr) * (SndHdrIndex + 1));

	if (NULL == NewHeader)
		return FALSE;

	NewHeaderName = (LPSTR)malloc(sizeof(CHAR) * (HeaderNameLen + 1));

	if (NULL == NewHeaderName)
		goto RELEASE;

	NewHeaderValue = (LPSTR)malloc(sizeof(CHAR) * (HeaderValueLen + 1));

	if (NULL == NewHeaderValue)
		goto RELEASE;

	memcpy(NewHeaderName, HeaderName, HeaderNameLen);
	memcpy(NewHeaderValue, HeaderValue, HeaderValueLen);

	NewHeaderName[HeaderNameLen] = NewHeaderValue[HeaderValueLen] = NULL;

	NewHeader[SndHdrIndex].Alloc = 3;	// Name(1-HI), Value(1-LO) 
	NewHeader[SndHdrIndex].Name = NewHeaderName;
	NewHeader[SndHdrIndex].Value = NewHeaderValue;
	pHeader->SendHeader = NewHeader;

	++pHeader->SendHeaderCount;

	return TRUE;

RELEASE:

	if (NewHeader)
		free(NewHeader);

	if (NewHeaderName)
		free(NewHeaderName);

	if (NewHeaderValue)
		free(NewHeaderValue);

	return FALSE;
}

VOID 
HTTP_FreeHeader(
	pHdr pHeader
)
{
	if (pHeader)
	{
		if (pHeader->SendHeaderCount > 0 && pHeader->SendHeader)
		{
			for (DWORD Index = 0; Index < pHeader->SendHeaderCount; ++Index)
			{
				if (pHeader->SendHeader[Index].Alloc & 0x02)
					FREE_DATA(pHeader->SendHeader[Index].Name);

				if (pHeader->SendHeader[Index].Alloc & 0x01)
					FREE_DATA(pHeader->SendHeader[Index].Value);
			}

			FREE_DATA(pHeader->SendHeader);
			pHeader->SendHeaderCount = 0;
		}

		FREE_DATA(pHeader->RequestHeader);
		FREE_DATA(pHeader->RequestURLQuery);
	}
}

BOOLEAN
HTTP_ExtUploadProgress(
	PSOCKETEX		ClientSckArray,
	DWORD			MaxClinet,
	pHdr			pHeader,
	QWORD			&TotalRead,
	QWORD			&ContentLength
)
{
	LPSTR		upSession = NULL;

	if (NULL == ClientSckArray ||
		NULL == pHeader)
		return FALSE;

	if (pHeader->RequestURLQuery && pHeader->RequestURLQueryCount > 0)
	{
		if (upSession = FindVarNameValue(pHeader->RequestURLQuery, pHeader->RequestURLQueryCount, "s"))
		{
			for (UINT Index = 0; Index < MaxClinet; ++Index)
			{
				if (ClientSckArray[Index].sck &&
					ClientSckArray[Index].Session &&
					0 == strncmp(ClientSckArray[Index].Session, upSession, strlen(ClientSckArray[Index].Session)))
				{
					if (ClientSckArray[Index].pHeader)
					{
						if (ClientSckArray[Index].pHeader->TotalRead)
							TotalRead = *ClientSckArray[Index].pHeader->TotalRead;

						if (ClientSckArray[Index].pHeader->ContentLength)
							ContentLength = *ClientSckArray[Index].pHeader->ContentLength;

						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

BOOLEAN
HTTP_SetSckResponeSession(
	PSOCKETEX		psckEx,
	pHdr			pHeader
)
{
	LPSTR		upSession = NULL;

	if (NULL == psckEx ||
		NULL == pHeader)
		return FALSE;

	if (pHeader->RequestURLQuery && pHeader->RequestURLQueryCount > 0)
	{
		if (upSession = FindVarNameValue(pHeader->RequestURLQuery, pHeader->RequestURLQueryCount, "s"))
			psckEx->Session = upSession;
	}

	return TRUE;
}

LPSTR
HTTP_ExtUploadFileName(
	LPSTR pHeaderStart
)
{
	LPSTR		pTmp;
	pReqVar		ContentType = NULL;
	DWORD		ContentTypeCount;
	ReqHdr		RequestHeader[5];
	DWORD		RequestHeaderCount = 0;

	if (NULL == pHeaderStart)
		return NULL;

	if (FALSE == ParseHeader(pHeaderStart, RequestHeader, RequestHeaderCount))
		return NULL;

	if (NULL == (pTmp = FindVarNameValue(RequestHeader, RequestHeaderCount, "Content-Disposition")))
		return NULL;

	if (FALSE == AnalyzeCookie(pTmp, ContentType, ContentTypeCount))
		return NULL;

	pTmp = FindVarNameValue(ContentType, ContentTypeCount, "filename");

	FREE_DATA(ContentType);

	return pTmp;
}

BOOLEAN
HTTP_Upload(
	PSOCKETEX	psckEx,
	pHdr		pHeader,
	LPCSTR		UploadPath
)
{
	SOCKET		sck;
	QWORD		ContentLength;		// Content의 총 길이
	QWORD		TotalRead;			// Content를 얼마나 읽었는지 나타내는 변수 ( 총 읽은 길이 )
	DWORD		ContentTypeCount;	// 요청 받은 헤더 중 Content-Type 헤더의 타입 수
	DWORD		ContentRead;		// AnalyzeHeader 함수에서 실수로 Content 내용까지 읽었을 경우 얼마나 읽었는지 나타내는 변수
	DWORD		SectorRead;			// 섹터 당 얼마나 패킷을 읽었는지 나타내는 변수
	DWORD		BoundaryLen;		// Boundary의 길이를 나타내는 변수
	DWORD		BytesWritten;		// buffer 에 담긴 파일내용을 얼마나 쓸 것인지 또는 얼마나 쓰였는지 나타내는 변수
	DWORD		dwBytesToRead;		// 패킷을 얼마나 읽을 것인지 나타내는 변수
	LPSTR		Buffer;				// 버퍼 ( 임시공간 )
	LPSTR		Boundary;			// 경계
	LPSTR		pBoundary;			// 버퍼에서 경계 문자열을 찾을 경우 그 위치를 나타내는 변수
	LPSTR		pBoundaryEnd;		// 버퍼에서 경계를 포함한 헤더의 끝을 알려주는 변수
	LPSTR		pPos;				// 버퍼에서 헤더의 첫 시작의 위치를 알려주는 변수
	LPSTR		szFileName;			// 버퍼에서 파싱한 파일 이름을 나타내는 변수
	pReqVar		ContentType;		// 요청 받은 헤더의 이름과 그 값들을 알려주는 구조체
	BOOLEAN		ContentEnd;			// Content가 끝났으면 TRUE의 값을 가지고 아니라면 FALSE 의 값을 가지는 변수
	BOOLEAN		BoundaryFound;		// Content 에서 Boundary(경계) 를 찾을 경우 TRUE 아니라면 FALSE 값을 가지는 변수
	BOOLEAN		FileDownload;		// 파일을 다운로드 할 준비가 완료되면 TRUE 아니면 FALSE
	BOOLEAN		bResult = FALSE;	// 이 프로시져의 리턴 값을 결정짓는 변수 ( 성공적으로 업로드가 끝나면 TRUE 아니면 FALSE )
	BOOLEAN		OnceRead = FALSE;	// 소켓으로 부터 버퍼에 데이터를 담고 데이터의 끝이 경계 시작문자(-) 이면 TRUE 아니면 FALSE
	HANDLE		hFile = INVALID_HANDLE_VALUE;
	CHAR		szFullPath[MAX_PATH];

	if (NULL == psckEx ||
		NULL == psckEx->pHeader ||
		NULL == pHeader)
		return FALSE;

	sck = psckEx->sck;

	if (0 >= pHeader->RequestHeaderCount ||
		NULL == pHeader->RequestHeader)
		return FALSE;

	if (NULL == (Buffer = FindHeaderValue(pHeader, "Content-Length")))
		return FALSE;

	ContentLength = strtoull(Buffer, &Boundary, 10);	// Boundary : 아무 의미없음

	if (NULL == (Buffer = FindHeaderValue(pHeader, "Content-Type")))
		return FALSE;

	if (FALSE == AnalyzeCookie(Buffer, ContentType, ContentTypeCount))
		return FALSE;

	Boundary = FindVarNameValue(ContentType, ContentTypeCount, "boundary");

	FREE_DATA(ContentType);

	if (NULL == Boundary)
		return FALSE;

	if (NULL == (Buffer = (LPSTR)malloc((MAX_FILEBUF + 1) * sizeof(CHAR))))
		return FALSE;

	// Boundary 문자열 앞에 '--' 를 더 추가한다.
	Boundary -= 2;
	Boundary[0] = Boundary[1] = '-';

	BoundaryLen = strlen(Boundary);

	// AnalyzeHeader 함수가 Content 를 읽었을 경우 Buffer에 Content 값을 복사한다.
	ContentRead = pHeader->ContentRead;
	if (ContentRead > 0)
		memcpy(Buffer, pHeader->HeaderEnd, ContentRead);

	// 이미 앞서 읽은 패킷이 있으므로 TotalRead에 읽은 수만큼 플러스 시킨다.
	TotalRead = ContentRead;
	SectorRead = ContentRead;

	FileDownload = FALSE;
	ContentEnd = FALSE;

	dwBytesToRead = MAX_FILEBUF - ContentRead - BoundaryLen - 3;

	psckEx->pHeader->TotalRead = &TotalRead;
	psckEx->pHeader->ContentLength = &ContentLength;

	while (true)
	{
		DWORD amountReads = 0;
		
		if (ContentLength > TotalRead)
		{
			if (FALSE == (amountReads = ReadSocketContent(sck, Buffer + ContentRead,
				dwBytesToRead, TotalRead, ContentLength)))
				goto RELEASE;
		}
		else
		{
			goto RELEASE;
		}

		ContentRead = 0;
		SectorRead += amountReads;
	
		BoundaryFound = FALSE;

		// Buffer의 맨 마지막 문자가 '-' 이면 뒤에 1바이트를 더 읽어 무슨 문자인지 판별한다.
		if (Buffer[amountReads - 1] == '-' && 
			ContentLength > TotalRead)
		{
			if (FALSE == (amountReads = ReadSocketContent(sck, Buffer + amountReads,
				1, TotalRead, ContentLength)))
				goto RELEASE;

			OnceRead = TRUE;
			SectorRead += amountReads;
		}

		pBoundary = Buffer;

		while (pBoundary)
		{
			BoundaryFound = FALSE;

			while (pBoundary = (LPSTR)memchr(pBoundary, '-', SectorRead - (pBoundary - Buffer)))
			{
				if (pBoundary[1] == '-')
				{
					// 더 이상 읽을 바이트가 없으면 다시 갱신
					if (pBoundary - Buffer + BoundaryLen + 3 - OnceRead >= SectorRead)
					{
						if (FALSE == (amountReads = ReadSocketContent(sck, pBoundary + SectorRead - (pBoundary - Buffer),
							BoundaryLen + 3 - OnceRead, TotalRead, ContentLength)))
							goto RELEASE;

						OnceRead = FALSE;
						SectorRead += amountReads;
					}

					// 만약 Boundary를 찾으면
					if (0 == strncmp(pBoundary, Boundary, BoundaryLen))
					{
						BoundaryFound = TRUE;
						ContentEnd = *(pBoundary + BoundaryLen) == '-';

						break;
					}
				}
				
				pBoundary += 2;
			}

			if (BoundaryFound)
			{
				// 파일 다운로드 중이였다면
				if (FileDownload)
				{
					BytesWritten = pBoundary - pBoundaryEnd - 2;

					WriteFile(hFile, pBoundaryEnd, BytesWritten, &BytesWritten, NULL); // 파일에 기록한다.
					CloseHandle(hFile); // 파일을 저장한다.

					hFile = INVALID_HANDLE_VALUE;
					FileDownload = FALSE;
				}

				//  Content 가 끝나면 Upload Procedure를 종료한다.
				if (ContentEnd)
				{
					bResult = TRUE;
					goto RELEASE;
				}

				// 만약 헤더의 끝 문자열을 찾을 수 없을 경우
				if (NULL == (pBoundaryEnd = strstr(pBoundary, "\r\n\r\n")))
				{
					DWORD AlreadyReads = SectorRead - (pBoundary - Buffer);

					memcpy(Buffer, pBoundary, AlreadyReads); // Boundary가 있는곳부터 복사한다

					dwBytesToRead = MAX_FILEBUF - BoundaryLen - 3;

					if (FALSE == (amountReads = ReadSocketContent(sck, Buffer + AlreadyReads,
						dwBytesToRead - AlreadyReads, TotalRead, ContentLength)))
						goto RELEASE;

					SectorRead = AlreadyReads + amountReads;
					pBoundary = Buffer; // 처음부터 Boundary 의 문자열이 Buffer안에 들어가 있으므로 포인터 변수에 시작주소를 넣는다

					// 그래도 못찾으면 접속종료 시킨다. ( 비정상 )
					if (NULL == (pBoundaryEnd = strstr(Buffer, "\r\n\r\n")))
						goto RELEASE;
				}

				// 바로 헤더 파싱을 시작한다.
				*pBoundaryEnd = NULL;

				pBoundaryEnd += 4;
				pPos = pBoundary + BoundaryLen;
				SKIP_RETURNCHAR(pPos);

				// 파일 이름을 추출한다.
				if (NULL == (szFileName = HTTP_ExtUploadFileName(pPos)))
					goto RELEASE;

				// 파일 이름에 "가 포함되있는지 확인한다.
				if (szFileName[0] == '"')
				{
					++szFileName;
					szFileName[strlen(szFileName) - 1] = NULL;
				}

				// UTF-8을 ANSI로 변환한다.
				UTF8toANSI_NoAlloc(szFileName, szFileName);

				// 파일 이름에 들어가선 안되는 문자가 있는지 검사 후 재배치 한다.
				RemoveSpecialChar(szFileName, strlen(szFileName));

				// 저장될 폴더 경로와 위에서 추출한 문자열이랑 합친다.
				sprintf_s(szFullPath, sizeof(szFullPath), "%s\\%s", UploadPath, szFileName);

				// 중복 파일 체크
				if ((DWORD)-1 != GetFileAttributes(szFullPath))
				{
					LPSTR Ext = szFileName;
					BOOLEAN ctExt = FALSE;
					GETFILEEXT(Ext);

					if (Ext)
					{
						--Ext;
						*Ext = NULL;
						ctExt = TRUE;
					}

					for (DWORD Index = 1; Index < (DWORD)-1; ++Index)
					{
						if (ctExt)
							sprintf_s(szFullPath, sizeof(szFullPath), "%s\\%s (%d).%s", UploadPath, szFileName, Index, Ext + 1);
						else 
							sprintf_s(szFullPath, sizeof(szFullPath), "%s\\%s (%d)", UploadPath, szFileName, Index);

						if ((DWORD)-1 == GetFileAttributes(szFullPath))
							break;
					}
				}

				// CreateFile API로 파일을 생성한다.
				hFile = CreateFileA(
					szFullPath,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);

				if (INVALID_HANDLE_VALUE == hFile)
					goto RELEASE;

				FileDownload = TRUE;
				pBoundary = pBoundaryEnd;
			}
			else if (FileDownload)
			{
				BytesWritten = SectorRead - (pBoundaryEnd - Buffer);
				WriteFile(hFile, pBoundaryEnd, BytesWritten, &BytesWritten, NULL); 
			}
		}

		SectorRead = 0;
		pBoundaryEnd = Buffer;
		dwBytesToRead = MAX_FILEBUF - BoundaryLen - 3;
	}


RELEASE:

	FREE_DATA(Buffer);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);

		if (FALSE == bResult)
			DeleteFile(szFullPath);
	}

	return bResult;
}

BOOLEAN 
HTTP_ContextFile(
	SOCKET		sck,
	LPCSTR		Path,
	LPCSTR		RelativePath,
	BYTE		ShUploadAndChat
)
{
	WIN32_FIND_DATAA	fd;
	CHAR				PATH[MAX_PATH];
	size_t				len = 0;
	HANDLE				hFind;
	LPSTR				utf8 = NULL;

	if (NULL == RelativePath)
	{
		sprintf_s(PATH, sizeof(PATH), "%s\\*.*", Path);
		RelativePath = "/";
		len = 1;
	}
	else
	{
		len = strlen(RelativePath);

		if (strstr(RelativePath, "..") || 0 == len)
		{
			sprintf_s(PATH, sizeof(PATH), "%s\\*.*", Path);
			RelativePath = "/";
			len = 1;
		}
		else
		{
			if (RelativePath[len - 1] == '/' || RelativePath[len - 1] == '\\')
				sprintf_s(PATH, sizeof(PATH), "%s\\%s*.*", Path, RelativePath);
			else
				sprintf_s(PATH, sizeof(PATH), "%s\\%s\\*.*", Path, RelativePath);
		}
	}

	if (0xFFFFFFFF != GetFileAttributes(PATH))
		return FALSE;

	if (INVALID_HANDLE_VALUE == (hFind = FindFirstFileA(PATH, &fd)))
		return FALSE;

	ANSItoUTF8(RelativePath, utf8);

	HTTP_SendClientText(sck, 
		"<html>"
		"<head>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
		"<title>Index of %s</title>"
		"</head>"
		"<body>"
		"<h1>Index of %s</h1>", 
		utf8, utf8);

	HTTP_SendClientText(sck,
		"<table>"
		"<tr>"
		"<th valign=\"top\"></th>"
		"<th>&nbsp;&nbsp;Name&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Last modified&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Size&nbsp;&nbsp;</th>"
		"<th>&nbsp;&nbsp;Description&nbsp;&nbsp;</th></tr>");

	HTTP_SendClientText(sck, "<tr><th colspan=\"10\"><hr></th></tr>");
	FREE_DATA(utf8);

	memcpy(PATH, RelativePath, MAX_PATH);

	for (int x = len; x >= 0; --x)
	{
		if (PATH[x] == '/')
		{
			PATH[x] = 0;
			break;
		}

	}

	PATH[len] = 0;

	HTTP_SendClientText(sck,
		"<tr>"
		"<td valign=\"top\"></td>"
		"<td>&nbsp;&nbsp;<a href=\"file?v=%s\">Parent Directory</a>&nbsp;&nbsp;</td>"
		"<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>"
		"<td align=\"center\">&nbsp;&nbsp;  - &nbsp;&nbsp;</td>"
		"<td>&nbsp;&nbsp;Directory&nbsp;&nbsp;</td>"
		"</tr>",
		PATH
	);

	do
	{
		if (*fd.cFileName != '.')
		{
			LPSTR			utf16 = NULL;
			SYSTEMTIME		stime;

			if (RelativePath[len - 1] == '/')
				sprintf_s(PATH, sizeof(PATH), "%s%s", RelativePath, fd.cFileName);
			else
				sprintf_s(PATH, sizeof(PATH), "%s/%s", RelativePath, fd.cFileName);

			ANSItoUTF8(fd.cFileName, utf8);
			utf16 = URLEncode(PATH);

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				HTTP_SendClientText(sck,
					"<tr>"
					"<td valign=\"top\"></td>"
					"<td>&nbsp;&nbsp;<a href=\"file?v=%s\">%s/</a>&nbsp;&nbsp;</td>"
					"<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>"
					"<td align=\"center\">&nbsp;&nbsp;  - &nbsp;&nbsp;</td>"
					"<td>&nbsp;&nbsp;Directory&nbsp;&nbsp;</td>"
					"</tr>",
					 utf16,
					 utf8
				);
			}
			else
			{
				CHAR Unit[30];

				FileTimeToSystemTime(&fd.ftLastWriteTime, &stime);
				SizeToUnit(Unit, sizeof(Unit), fd.nFileSizeHigh, fd.nFileSizeLow);

				HTTP_SendClientText(sck,
					"<tr>"
					"<td valign=\"top\"></td>"
					"<td>&nbsp;&nbsp;<a href=\"file?v=%s\">%s</a>&nbsp;&nbsp;</td>"
					"<td>&nbsp;&nbsp;%4d-%02d-%02d %02d:%02d:%02d&nbsp;&nbsp;</td>"
					"<td align=\"right\">&nbsp;&nbsp;%s&nbsp;&nbsp;</td>"
					"<td>&nbsp;&nbsp;File&nbsp;&nbsp;</td>"
					"</tr>",
					utf16,
					utf8,
					stime.wYear,		// TIME START
					stime.wMonth,
					stime.wDay,
					stime.wHour,
					stime.wMinute,
					stime.wSecond,		// TIME END
					Unit
				);
			}

			FREE_DATA(utf8);
			FREE_DATA(utf16);
		}
	}while (FindNextFileA(hFind, &fd));

	FindClose(hFind);

	HTTP_SendClientText(sck, 
		"<tr><th colspan=\"10\">"
		"<hr></th></tr>"
		"</table><div class=\"foot\">%s", SERVER_NAME);

	if (ShUploadAndChat & 0x01)
		HTTP_SendClientText_Pure(sck, 
			" - <a href='upload' onclick=\"window.open(this.href, '_blank', 'width=400px,height=100px,toolbars=no,scrollbars=no'); return false;\">Upload</a>", 142);

	if (ShUploadAndChat & 0x02)
		HTTP_SendClientText_Pure(sck,
			" - <a href='chat' onclick=\"window.open(this.href, '_blank', 'width=600px,height=600px,toolbars=no,scrollbars=no'); return false;\">Chat</a>", 138);

	HTTP_SendClientText_Pure(sck, "</div></body></html>", 20);

	return TRUE;
}

VOID 
HTTP_Context404(
	SOCKET		sck,
	pHdr		pHeader
)
{
	if (NULL == pHeader)
		return;

	pHeader->RequestURL = (LPSTR)".html";
	HTTP_SendClientHeader(sck, pHeader, 211);

	HTTP_SendClientText(sck, 
		"<html><head>"
		"<title>Not Found</title>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
		"</head><body><h1>Not Found</h1></body>"
	);
}

VOID
HTTP_ContextUpload(
	SOCKET		sck,
	pHdr		pHeader
)
{
	DWORD		UTF8Size;
	LPSTR		UTF8;

	LPCSTR		HTMLHardCode =
		"<html>"
		"    <head>"
		"        <title>Upload</title>"
		"        <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"
		"        <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
		"        <style>"
		"            #myProgress {"
		"                width: 100%;"
		"                background-color: #ddd;"
		"            }"
		"            #myBar {"
		"                width: 0%;"
		"                height: 30px;"
		"                background-color: #4CAF50;"
		"            }"
		"        </style>"
		"        <script>"
		"            function MakeSession(length) {"
		"                var result = '';"
		"                var characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';"
		"                var charactersLength = characters.length;"
		"                for (var i = 0; i < length; i++)"
		"                    result += characters.charAt(Math.floor(Math.random() * charactersLength));"
		"                return result;"
		"            }"
		"            function uploadform() {"
		"                var upform = document.upload;"
		"                session = MakeSession(10);"
		"                upform.action = 'upload?s=' + session;"
		"                upform.s = session;"
		"                upform.submit();"
		"                document.getElementById('dialog').innerHTML = \"<div id='myProgress'><div id='myBar'></div></div>\";"
		"                var a = 0;"
		"                setInterval(function() {"
		"                    var b = new XMLHttpRequest();"
		"                    b.onreadystatechange = function() {"
		"                        if (this.status == 200 && this.readyState == this.DONE) {"
		"                            var d = JSON.parse(b.responseText);"
		"                            var e = document.getElementById('myBar');"
		"                            var c = d.pos / d.length * 100;"
		"                            e.style.width = c + '%';"
		"                        }"
		"                    }"
		"                    ,"
		"                    b.open('GET', './upstatus?s=' + session, !0),"
		"                    b.send();"
		"                }, 500);"
		"            }"
		"            var session;"
		"        </script>"
		"    </head>"
		"    <body>"
		"        <div id='dialog'>"
		"            <fieldset id='upload'>"
		"                <form name='upload' method='post' enctype='multipart/form-data' style='width=device-width;'>"
		"                    <input type='file' name='filename' multiple style='display:block;'/>"
		"                    <input type='button' value='Upload' style='margin-top:0.5em;' onclick='uploadform()'/>"
		"                </form>"
		"            </fieldset>"
		"        </div>"
		"    </body>"
		"</html>";

	if (NULL == pHeader)
		return;

	pHeader->RequestURL = (LPSTR)".html";

	if (0 == (UTF8Size = ANSItoUTF8(HTMLHardCode, UTF8)))
		return;

	HTTP_SendClientHeader(sck, pHeader, UTF8Size);
	HTTP_SendClientText_Pure(sck, UTF8, UTF8Size);

	FREE_DATA(UTF8);
}

VOID
HTTP_ContextChat(
	SOCKET		sck,
	pHdr		pHeader,
	LPCSTR		ServerIP
)
{
	DWORD		UTF8Size;
	LPSTR		UTF8 = NULL;
	LPSTR		ReplaceString = NULL;
	extern HWND	___MyDialoghWnd;

	LPCSTR		HTMLHardCode =
		"<html>"
		"    <head>"
		"        <title>Chat</title>"
		"        <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"
		"        <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
		"        <style type='text/css'>"
		"            #chat {"
		"                border: solid 1px #999999;"
		"                border-top-color: #CCCCCC;"
		"                border-left-color: #CCCCCC;"
		"                padding: 5px;"
		"                width: 99%;"
		"                height: 90%;"
		"                overflow-y: scroll;"
		"            }"
		"            #sendMessage {"
		"                width: 75%;"
		"            }"
		"            #send {"
		"                width: 20%;"
		"            }"
		"        </style>"
		"        <script language='javascript' type='text/javascript'>"
		"            function testWebSocket() {"
		"                websocket = new WebSocket('ws://%%%SERVER_IP%%%/sock'),"
		"                websocket.onclose = function(a) {"
		"                    onClose(a);"
		"                }"
		"                ,"
		"                websocket.onmessage = function(a) {"
		"                    onMessage(a);"
		"                }"
		"                ;"
		"            }"
		"            function init() {"
		"                var a = document.getElementById('sendMessage');"
		"                output = document.getElementById('chat'),"
		"                a.onkeypress = function(b) {"
		"                    b || (b = window.event);"
		"                    var c = b.keyCode || b.which;"
		"                    return c == '13' ? (doSend(a.value),"
		"                    a.value = '',"
		"                    !1) : void 0;"
		"                }"
		"                ;"
		"                var b = document.getElementById('send');"
		"                b.onclick = function() {"
		"                    doSend(a.value),"
		"                    a.value = '';"
		"                }"
		"                ;"
		"                var c = document.getElementById('cls');"
		"                c.onclick = function() {"
		"                    output.innerHTML = '';"
		"                }"
		"                ,"
		"                testWebSocket();"
		"            }"
		"            function onClose(a) {"
		"                writeToScreen('', '서버와의 연결이 끊어졌습니다.');"
		"            }"
		"            function onMessage(a) {"
		"                a.data == 'PING' ? websocket.send('PONG') : writeToScreen('', a.data);"
		"            }"
		"            function doSend(a) {"
		"                if (a.length > 2000) {"
		"                    writeToScreen('red', '2000 자 미만으로 메세지 작성 바랍니다.');"
		"                    return;"
		"                }"
		"                a.length > 0 && websocket.send(a);"
		"            }"
		"            function writeToScreen(b, c) {"
		"                var a = document.createElement('p');"
		"                a.style.marginBlockStart = '0',"
		"                a.style.marginBlockEnd = '0',"
		"                b != '' && (a.style.color = b),"
		"                a.innerHTML = c,"
		"                output.appendChild(a),"
		"                output.scrollTop = output.scrollHeight;"
		"            }"
		"            var output;"
		"            window.addEventListener('load', init, !1);"
		"        </script>"
		"    </head>"
		"    <body>"
		"        <div id='chat'></div>"
		"        <input class='draw-border' id='sendMessage' size='100%' value=''>"
		"        <button class='echo-button' id='send' class='wsButton'>보내기</button>"
		"        <button class='echo-button' id='cls' class='wsButton'>대화클리어</button>"
		"    </body>"
		"</html>";

	if (NULL == pHeader)
		return;

	pHeader->RequestURL = (LPSTR)".html";

	if (FALSE == Replace(ReplaceString, (LPSTR)HTMLHardCode, "%%%SERVER_IP%%%", ServerIP))
		return;

	if (0 == (UTF8Size = ANSItoUTF8(ReplaceString, UTF8)))
		goto RELEASE;

	HTTP_SendClientHeader(sck, pHeader, UTF8Size);
	HTTP_SendClientText_Pure(sck, UTF8, UTF8Size);

	FREE_DATA(UTF8);

RELEASE:
	FREE_DATA(ReplaceString);
}