#ifndef SERVERLOGIC_H
#define SERVERLOGIC_H

#include "Socket.h"
#include "WebSocket.h"
#include "union.h"

DWORD WINAPI
ProcessDataThread(
	LPVOID psck
);

#endif
