#include "WebSocket.h"

extern SOCKETEX		ClientSck[MAX_USER];
DWORD				ClientCount = 0;
BOOLEAN				CreatePingThread = FALSE;

BOOLEAN
WebSocket_Send(
	PSOCKETEX		psckEx,
	LPSTR			Buffer,
	DWORD			Len
);

VOID
WebSocket_SendData(
	PSOCKETEX		psckEx,
	_Opcode			PayloadType,
	LPCSTR			Buffer,
	QWORD			BufSize
);

VOID
WebSocket_NewConnection(
	PSOCKETEX		psckEx
);

VOID
WebSocket_Message(
	PSOCKETEX		psckEx,
	LPCSTR			Msg,
	DWORD			Len
);

DWORD WINAPI
WebSocket_Ping(
	LPVOID			arg
)
{
	BOOLEAN			Found;
	DWORD			Index;

	while (ClientCount)
	{
		Sleep(60000);

		Found = FALSE;
		for (Index = 0; Index < MAX_USER; ++Index)
		{
			if (ClientSck[Index].Type == SOCK_WEBSOCK &&
				ClientSck[Index].sck > 0)
			{
				ClientSck[Index].RcvPong = 1;
				WebSocket_SendData(ClientSck + Index, UTF_8, "PING", 4);
				Found = TRUE;
			}
		}
		
		if (FALSE == Found)
			break;

		Sleep(5000);

		for (Index = 0; Index < MAX_USER; ++Index)
		{
			if (ClientSck[Index].Type == SOCK_WEBSOCK &&
				ClientSck[Index].sck > 0 &&
				ClientSck[Index].RcvPong)
			{
				CloseSocket(ClientSck + Index);
			}
		}

	}

	CreatePingThread = FALSE;
	return FALSE;
}

BOOLEAN 
WebSocket_HandShake(
	PSOCKETEX		psckEx, 
	pHdr			pHeader
)
{
	CHAR			Key[70] = { 0 };
	CHAR			Result[100] = { 0 };
	CHAR			Hdr[300];
	LPSTR			pValue = NULL;
	LONG			bytesTransferred;
	DWORD			Len;
	DWORD			TotalTrnasferred;
	time_t			_time;
	struct tm		_tm;
	INT				nTimeOutValue = 3000;

	if (NULL == psckEx)
		return FALSE;

	// 1. HTTP 헤더에서 base64( 임의의 문자열 16바이트 ) 인코딩 된 Key 값을 파싱합니다.
	if (NULL == (pValue = FindHeaderValue(pHeader, "Sec-WebSocket-Key")))
		return FALSE;

	Len = strlen(pValue);
	memcpy(Key, pValue, Len);

	// 2. Key 값에 GUID(Magic String)값을 더해, 더한값을 SHA1 Hash 합니다.
	memcpy(Key + Len, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
	Len += 36;

	SHA1(Result, Key, Len);

	// 3. SHA1 Hash 된 값을 2바이트 씩 쪼갠다음 해당 Hex값을 -> Decimal로 변환 후 Ascii 로 변환합니다. ( 이미 끝냄 )
	// 4. 3. 의 결과 값을 base64 인코딩 합니다.
	memset(Key, 0, sizeof(Key));
	__base64_encode(Key, Result, 20);

	Len = sprintf_s(Hdr, sizeof(Hdr), "HTTP/1.1 %s\n", REQUEST_HEADER_101);
	time(&_time);	// 현재 시간을 불러옵니다.
	gmtime_s(&_tm, &_time); // 현재 시간을 GMT 시간으로 바꿔줍니다.
	strftime(Result, sizeof(Result), "%a, %d %b %Y %T", &_tm); // HTTP 헤더에 알맞는 시간 포맷으로 변환시킵니다.
	Len += sprintf_s(Hdr + Len, sizeof(Hdr) - Len, "Date: %s GMT\n", Result);
	Len += sprintf_s(Hdr + Len, sizeof(Hdr) - Len, "Connection: Upgrade\n");
	Len += sprintf_s(Hdr + Len, sizeof(Hdr) - Len, "Upgrade: websocket\n");
	Len += sprintf_s(Hdr + Len, sizeof(Hdr) - Len, "Sec-WebSocket-Accept: %s\n", Key);
	Len += sprintf_s(Hdr + Len, sizeof(Hdr) - Len, "Server: %s\n\n", SERVER_NAME);

	bytesTransferred = 0;
	TotalTrnasferred = 0;

	while (TotalTrnasferred < Len)
	{
		if (SOCKET_ERROR == (bytesTransferred = send(psckEx->sck, Hdr + TotalTrnasferred, (LONG)Len - TotalTrnasferred, 0)))
			return FALSE;

		TotalTrnasferred += bytesTransferred;
	}

	psckEx->Type = SOCK_WEBSOCK;
	
	if (SOCKET_ERROR == setsockopt(psckEx->sck, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nTimeOutValue, sizeof(nTimeOutValue)))
		return FALSE;

	if (SOCKET_ERROR == setsockopt(psckEx->sck, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeOutValue, sizeof(nTimeOutValue)))
		return FALSE;

	if (NULL == (psckEx->Session = (LPSTR)malloc(sizeof(CHAR) * (WEBSERV_NICKLEN + 1))))
		return FALSE;

	MakeRandomString(psckEx->Session, WEBSERV_NICKLEN);
	WebSocket_NewConnection(psckEx);

	if (FALSE == CreatePingThread)
	{
		CreatePingThread = TRUE;
		CreateThread(NULL, 0, WebSocket_Ping, NULL, 0, NULL);
	}

	return TRUE;
}

BOOLEAN
WebSocket_Read(
	PSOCKETEX		psckEx,
	MUST_FREE_MEMORY LPSTR &Buffer,
	DWORD			ReadSize,
	BOOLEAN			AutoFree
)
{
	LPSTR			lpszbuf = NULL;
	LONG			bytesReceived = 0;
	DWORD			amountReads = 0;
	myfd_set		fd;
	struct timeval	timeout;

	if (NULL == psckEx)
		return FALSE;

	fd.fd_array[0] = psckEx->sck;

	timeout.tv_sec = 1;
	timeout.tv_usec = 1000;

	if (AutoFree)
	{
		FREE_DATA(Buffer);

		 // 문자열은 무조건 길이 + 1 만큼 할당 해줘야한다. (NULL 문자)
		if (NULL == (lpszbuf = (LPSTR)malloc(sizeof(CHAR) * (ReadSize + 1))))
			return FALSE;

		Buffer = lpszbuf;
	}
	else
	{
		lpszbuf = Buffer;
	}

	do
	{
		fd.fd_count = 1;
		int res = select(1, (fd_set*)&fd, NULL, NULL, &timeout); // 소켓에 읽을 데이터가 있는지 관찰한다

		if (res > 0) // 읽을 데이터가 있으면,
		{
			if ((bytesReceived = recv(psckEx->sck, lpszbuf + amountReads, ReadSize - amountReads, 0)) == SOCKET_ERROR)
			{
				// 에러처리

				if (WSAGetLastError() != WSAETIMEDOUT) // WSAETIMEDOUT : 시간의 경과가 SO_RCVTIMEO 의 값을 넘어섰기 때문에, 예외로 둔다.
					goto RELEASE;

				continue;
			}
			else if (bytesReceived == 0) // 소켓 버퍼에 읽을 데이터가 있음에도 불구하고, 읽은 데이터의 수가 0이면 접속종료로 판단
				goto RELEASE;

			amountReads += bytesReceived;
		}
		else if (res < 0)
			goto RELEASE;

	} while (ReadSize > amountReads);

	return TRUE;

RELEASE:

	if (AutoFree)
		FREE_DATA(Buffer);

	return FALSE;

}

BOOLEAN
WebSocket_Send(
	PSOCKETEX		psckEx,
	LPSTR			Buffer,
	DWORD			Len
)
{
	DWORD			TotalTransferred = 0;
	LONG			bytesTransferred = 0;
	BOOLEAN			bResult = FALSE;

	if (NULL			== psckEx ||
		INVALID_SOCKET	== psckEx->sck ||
		0				== Len)
		return FALSE;

	while (TotalTransferred < Len)
	{
		if (0 >= (bytesTransferred = send(psckEx->sck, Buffer + TotalTransferred, Len - TotalTransferred, 0)))
			goto RELEASE;

		TotalTransferred += bytesTransferred;
	}

	bResult = TRUE;

RELEASE:

	return bResult;
}

VOID 
WebSocket_SendData(
	PSOCKETEX		psckEx,
	_Opcode			PayloadType, 
	LPCSTR			Buffer, 
	QWORD			BufSize
)
{
	sckDataFrame	Frame;
	CHAR			Hdr[8];
	DWORD			HdrLen = 0;
	LPSTR			sndData = NULL;

	if (NULL == psckEx)
		return;

	Frame.FIN = TRUE; // 데이터의 끝을 나타내는 플래그 입니다.
	Frame.OpCode = PayloadType; // 데이터의 종류를 나타내는 값 입니다.
	Frame.MASK = FALSE; // Client->Server 로 전달되는 메세지는 이 값이 항상 TRUE 어야 하며, 
	//이 값이 참이면 별도의 과정을 통해 Pure한 메세지를 얻을 수 있습니다.
	Frame.RSV = 0; // 미래를 위해 예약된 플래그 입니다.

	/* 
	WebSocket의 데이터 길이 계산법은 약간의 복잡한 절차를 거쳐 수신자에게 보내지게 되며, 그 과정은 다음과 같습니다.
	1. 데이터의 길이가 125 이하 이면, 별도의 과정 없이 그대로 보내면 됩니다.
	2. 126 이상이고 65535 이하 이면, 2 개의 바이트에 실제 데이터의 길이를 넣습니다.
	3. 65536 이상 이라면, 8 개의 바이트에 실제 데이터의 길이를 넣습니다.
	4. 모든 과정이 완료 되면, Payload Length 의 값은 각각 1 번(그대로), 2 번(126), 3 번(127) 이 됩니다.
	*/

	if (UTF_8 == PayloadType)
	{
		if (0 == (BufSize = ANSItoUTF8(Buffer, sndData)))
			return;

		--BufSize;
	}
	else
	{
		sndData = (LPSTR)Buffer;
	}

	if (BufSize < 0x7E)
	{
		Frame.PayloadLength = (BYTE)BufSize;
	}
	else if (BufSize >= 0x7E && BufSize <= 0xFFFF)
	{
		Frame.PayloadLength = 126;

		Hdr[0] = (BufSize >> 8) & 0xFF;
		Hdr[1] = BufSize & 0xFF;

		HdrLen = 0x02;
	}
	else
	{
		Frame.PayloadLength = 127;

		for (UINT i = 0; i < 8; ++i)
			Hdr[i] = (BufSize >> ((7 - i) * 8)) & 0xFF;

		HdrLen = 0x08;
	}

	WebSocket_Send(psckEx, (LPSTR)&Frame.Data, 2);

	if (HdrLen)
		WebSocket_Send(psckEx, Hdr, HdrLen);

	WebSocket_Send(psckEx, sndData, (INT)BufSize);

	if (UTF_8 == PayloadType)
		free(sndData);
}

VOID 
WebSocket_ReceiveData(
	PSOCKETEX		psckEx
)
{
	CHAR			MaskData[4];
	LPSTR			Packet = NULL;
	LPSTR			RealPayload = NULL;
	LPSTR			MaskPacket = NULL;
	DWORD			PayloadLength;	// WebSocket의 PayloadLength를 나타내는 패킷은 0 ~ 18,446,744,073,709,551,615(2^64-1) 까지 지원한다. 이 프로그램은 간단하게 4바이트만(2^32-1) 사용하여 프로그래밍 되었다.
	DWORD			RealPayloadLength = 0;
	sckDataFrame	Frame;
	sckDataFrame	Frag;
	BOOLEAN			Continuous = FALSE;

	MaskPacket = MaskData;

	while (psckEx->sck)
	{
		PayloadLength = 0;
		Packet = (LPSTR)&Frame.Data;

		if (FALSE == WebSocket_Read(psckEx, Packet, 2, FALSE)) // No Alloc
			return;

		Packet = NULL;

		if (Close == Frame.OpCode)
			return;

		if (126 > Frame.PayloadLength)
		{
			PayloadLength = Frame.PayloadLength;
		}
		else if (126 == Frame.PayloadLength)
		{
			if (FALSE == WebSocket_Read(psckEx, Packet, 2, TRUE)) // Alloc
				return;

			memcpy(&PayloadLength, Packet, 2);
			// 네트워크 바이트 순서를 호스트 바이트 순서로 바꾼다.
			PayloadLength = (DWORD)ntohs((u_short)PayloadLength);

			FREE_DATA(Packet);
		}
		else if (127 == Frame.PayloadLength)
		{
			if (FALSE == WebSocket_Read(psckEx, Packet, 8, TRUE))
				return;

			memcpy(&PayloadLength, Packet + 4, 4);
			PayloadLength = (DWORD)ntohl(PayloadLength);

			FREE_DATA(Packet);
		}
		else if (0 == Frame.PayloadLength)
		{
			if (Frame.MASK && (FALSE == WebSocket_Read(psckEx, MaskPacket, 4, FALSE)))
				return;

			continue;
		}

		// 채팅 메세지가 4096 바이트가 넘어가면
		if (PayloadLength > 2048)
			return;

		if (Continuous)
		{
			if (NULL == (RealPayload = (LPSTR)realloc(RealPayload, RealPayloadLength + PayloadLength + 1)))
				return;

			Packet = RealPayload + RealPayloadLength;
		}

		if (Frame.MASK && (FALSE == WebSocket_Read(psckEx, MaskPacket, 4, FALSE)))
			return;

		if (FALSE == WebSocket_Read(psckEx, Packet, PayloadLength, !Continuous))
			return;

		if (Frame.MASK)
		{
			for (QWORD Index = 0; Index < PayloadLength; ++Index)
				Packet[Index] ^= MaskData[Index % 4];
		}

		if (FALSE == Frame.FIN)
		{
			if (FALSE == Continuous)
			{
				Continuous = TRUE;
				Frag = Frame;
				RealPayload = Packet;
			}

			RealPayloadLength += PayloadLength;
			continue;
		}

		if (Continuous)
		{
			Continuous = FALSE;
			Frame = Frag;
			Packet = RealPayload;
			PayloadLength += RealPayloadLength;
			RealPayloadLength = 0;
		}

		Packet[PayloadLength] = NULL;

		switch (Frame.OpCode)
		{
			case UTF_8:
			{
				LPSTR Ansi;

				if (FALSE == UTF8toANSI(Packet, Ansi))
				{
					FREE_DATA(Packet);
					return;
				}

				if (psckEx->RcvPong && 0 == strncmp(Ansi, "PONG", 4))
					psckEx->RcvPong = 0;
				else
					WebSocket_Message(psckEx, Ansi, PayloadLength);

				FREE_DATA(Ansi);
				break;
			}
			case Binary:
				break;
			case Continuation:
				break;
		}

		FREE_DATA(Packet);
	}
}

// WebSocket Events

VOID
WebSocket_NewConnection(
	PSOCKETEX		psckEx
)
{
	CHAR			ConnMsg[100];

	++ClientCount;

	sprintf_s(ConnMsg, sizeof(ConnMsg), 
		"익명_%s 님이 접속하셨습니다. (접속 인원 : %d)",
		psckEx->Session,
		ClientCount
	);

	//BroadCast
	for (UINT Index = 0; Index < MAX_USER; ++Index)
	{
		if (ClientSck[Index].sck && ClientSck[Index].Type == SOCK_WEBSOCK)
		{
			if (psckEx != ClientSck + Index &&
				ClientSck[Index].ip.S_un.S_addr == psckEx->ip.S_un.S_addr)
			{
				// 다중 클라이언트 사용자는 즉시 종료시킨다.
				shutdown(ClientSck[Index].sck, 0x02);
				continue;
			}

			WebSocket_SendData(ClientSck + Index, UTF_8, ConnMsg, 4096);
		}
	}
}

VOID 
WebSocket_ConnectionClose(
	PSOCKETEX		psckEx
)
{	
	CHAR			ConnMsg[100];

	--ClientCount;

	if (NULL == psckEx->Session)
		return;

	sprintf_s(ConnMsg, sizeof(ConnMsg), 
		"익명_%s 님이 접속종료하셨습니다. (접속 인원 : %d)",
		psckEx->Session,
		ClientCount
	);

	FREE_DATA(psckEx->Session);

	//BroadCast
	for (UINT Index = 0; Index < MAX_USER; ++Index)
	{
		if (ClientSck[Index].sck &&
			ClientSck[Index].Type == SOCK_WEBSOCK &&
			psckEx != ClientSck + Index)
		{
			WebSocket_SendData(ClientSck + Index, UTF_8, ConnMsg, 4096);
		}
	}
}

VOID 
WebSocket_Message(
	PSOCKETEX		psckEx,
	LPCSTR			Msg,
	DWORD			Len
)
{
	LPSTR			Message = NULL;
	UINT			spLen;

	//BroadCast
	for (UINT Index = 0; Index < MAX_USER; ++Index)
	{
		if (ClientSck[Index].sck && ClientSck[Index].Type == SOCK_WEBSOCK)
		{
			if (Message = (LPSTR)malloc(sizeof(CHAR) * (Len + WEBSERV_NICKLEN + 10)))
			{
				spLen = sprintf_s(Message, Len + WEBSERV_NICKLEN + 10, "익명_%s: ",
					psckEx->Session
				);

				memcpy(Message + spLen, Msg, Len);
				Message[Len + spLen] = NULL;
				Len = CovTag(Message);

				WebSocket_SendData(ClientSck + Index, UTF_8, Message, Len);
				free(Message);
			}
		}
	}
}

