#include "ServerLogic.h"

extern SOCKETEX ClientSck[MAX_USER];
extern BYTE UpAndChatActive;
extern CHAR	DOWNLOAD_PATH[MAX_PATH];
extern CHAR	UPLOAD_PATH[MAX_PATH];
extern CHAR	SERVER_IP[15];

DWORD WINAPI
ProcessDataThread(
	LPVOID lparg
)
{
	PSOCKETEX	psckEx = (PSOCKETEX)lparg;
	SOCKET		sck = psckEx->sck;

	LPSTR 		Buffer = NULL;
	LPSTR		pEndHdr;
	LPSTR		_Value = NULL;

	DWORD		ContentRead = 0;
	DWORD		URL_Len = 0;
	Hdr			MainHeader;

	if (NULL == lparg)
		goto DOWN;

	memset(&MainHeader, 0, sizeof(Header));

	if (FALSE == ReadSocketHeader(sck, Buffer, pEndHdr, &ContentRead))
		goto DOWN;

	if (FALSE == AnalyzeHeader(Buffer, &MainHeader, pEndHdr, ContentRead))
		goto DOWN;

	psckEx->pHeader = &MainHeader;

	URL_Len = strlen(MainHeader.RequestURL);

	switch (MainHeader.HdrMethods)
	{
		case _GET:
		{
			if (NULL == MainHeader.RequestURL ||
				NULL == *MainHeader.RequestURL ||
				0 == strncmp(MainHeader.RequestURL, "file", URL_Len))
			{
				if (_Value = FindVarNameValue(MainHeader.RequestURLQuery,
					MainHeader.RequestURLQueryCount, "v"))
					URLDecode(_Value);

				if (FALSE == HTTP_ContextFile(sck, DOWNLOAD_PATH, _Value, UpAndChatActive))
				{
					MainHeader.RequestURL = _Value;

					if (FALSE == HTTP_SendFile(sck, &MainHeader, DOWNLOAD_PATH))
						HTTP_Context404(sck, &MainHeader);
				}

				break;
			}
			else if (UpAndChatActive & 0x01 &&
					0 == strncmp(MainHeader.RequestURL, "upload", URL_Len))
			{
				HTTP_ContextUpload(sck, &MainHeader);
			}
			else if (UpAndChatActive & 0x01 && 
					0 == strncmp(MainHeader.RequestURL, "upstatus", URL_Len))
			{
				QWORD	querylen;
				QWORD	totalread;
				QWORD	ContentLength;
				CHAR	tmp[60];

				if (FALSE == HTTP_ExtUploadProgress(ClientSck, MAX_USER, &MainHeader, totalread, ContentLength))
					goto DOWN;

				querylen = sprintf_s(tmp, sizeof(tmp), "{\"pos\":%llu, \"length\":%llu}", totalread, ContentLength);

				HTTP_SendClientHeader(sck, &MainHeader, querylen);
				HTTP_SendClientText(sck, tmp, querylen);
			}
			else if (UpAndChatActive & 0x02 &&
					0 == strncmp(MainHeader.RequestURL, "chat", URL_Len))
			{
				HTTP_ContextChat(sck, &MainHeader, SERVER_IP);
			}
			else if (UpAndChatActive & 0x02 &&
				0 == strncmp(MainHeader.RequestURL, "sock", URL_Len) &&
				0 == strncmp(FindHeaderValue(&MainHeader, "Upgrade"), "websocket", 9)) // WebSocket
			{
				if (WebSocket_HandShake(psckEx, &MainHeader))
				{
					WebSocket_ReceiveData(psckEx);
					WebSocket_ConnectionClose(psckEx);
				}
			}
			else
			{
				HTTP_Context404(sck, &MainHeader);
			}

			break;
		}
		case _POST:
		{
			pReqVar		pQuery = NULL;
			LPSTR		Session = NULL;

			if (UpAndChatActive & 0x01 && 
				0 == strncmp(MainHeader.RequestURL, "upload", URL_Len))
			{
				HTTP_SetSckResponeSession(psckEx, &MainHeader);

				if (HTTP_Upload(psckEx, &MainHeader, UPLOAD_PATH))
					HTTP_ContextUpload(sck, &MainHeader);

				psckEx->Session = NULL;
			}

			break;
		}
	}

DOWN:

	HTTP_FreeHeader(&MainHeader);
	FREE_DATA(Buffer);
	CloseSocket(psckEx);

	return FALSE;
}