#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "Parser.h"
#include "base64.h"
#include "sha1.h"
#include "Unicode.h"
#include "Socket.h"
#include "union.h"

// FUNCTION
BOOLEAN
WebSocket_HandShake(
	PSOCKETEX psckEx,
	pHdr pHeader
);

VOID
WebSocket_ReceiveData(
	PSOCKETEX psckEx
);

VOID
WebSocket_ConnectionClose(
	PSOCKETEX psckEx
);

#endif