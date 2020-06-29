#ifndef SOCKET_H
#define SOCKET_H

// INCLUDE
#include "Parser.h"
#include "HTTP.h"
#include "union.h"

// FUNCTION
SOCKET 
InitializeSocket(
);

BOOLEAN
BindAndListenSocket(
	SOCKET			sck,
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout
);

VOID
CloseSocket(
	PSOCKETEX		psckEx
);

VOID
CloseSocket(
	SOCKET			sck
);

BOOLEAN
StartWebServer(
	WORD			wSrvPort,
	INT				send_timeout,
	INT				recv_timeout,
	DATAPROCESSPROC ProcessProc
);

BOOLEAN
StopWebServer(
);

BOOLEAN
ReadSocketHeader(
	SOCKET			sck,
	MUST_FREE_MEMORY LPSTR &Buffer,
	LPSTR			&pEnd,
	LPDWORD			lpContentRead
);

BOOLEAN
ReadSocketContent_Text(
	SOCKET			sck,
	MUST_FREE_MEMORY LPSTR &Buffer,
	DWORD			ReadSize,
	DWORD			Offset
);

DWORD
ReadSocketContent(
	SOCKET			sck,
	LPSTR			Buffer,
	DWORD			ReadSize,
	QWORD			&ReadPos,
	QWORD			ContentLength
);

BOOLEAN
GetHostIP(
	HWND	hComboBox
);

#endif