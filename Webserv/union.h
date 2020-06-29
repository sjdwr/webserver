#ifndef UNION_H
#define UNION_H

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// PRAGMA
#pragma comment ( lib, "ws2_32.lib" )

#define MAX_FILEBUF						1024 * 128
#define MAX_HDRBUF						1024
#define MAX_USER						1024
#define SERVER_NAME						"MyWebServ"

#define REQUEST_HEADER_101				"101 Switching Protocols" // HTTP/1.1 101 Web Socket Protocol Handshake
#define REQUEST_HEADER_200				"200 OK"
#define REQUEST_HEADER_206				"206 Partial Content"
#define REQUEST_HEADER_400				"400 Bad Request"

#define GETFILENAME(sz)					for(int i=strlen(sz);i>=0;--i){if(sz[i]=='/'){sz=sz+i+1;break;}}
#define GETFILEEXT(sz)					do{int __xd__=0;for(int i=strlen(sz);i>=0;--i){if(sz[i]=='.'){sz=sz+i+1;__xd__=1;break;}}if(!__xd__)sz=0;}while(0)

#define SKIP_RETURNCHAR(p)				for(;(*p=='\r'||*p=='\n');++p)*p=0;
#define SKIP_SPACECHAR(p)				for(;*p==' ';++p)*p=0;
#define FREE_DATA(p)					if(p) {free(p); p=NULL;}
#define EXT_PAYLOAD_LENGTH_M(b1, b2)	((unsigned char)b1 << 8 | (unsigned char)b2)

#define PATH_ROOT						1
#define PATH_FILE						2

#define SOCK_HTTP						1
#define SOCK_WEBSOCK					2
#define MUST_FREE_MEMORY

#define WEBSERV_STOP					0
#define WEBSERV_START					1
#define WEBSERV_END						2

#define WEBSERV_NICKLEN					7

// ENUM
enum methods
{
	_GET = 0,
	_POST,
	_HEAD,
	_PUT,
	_PATCH,
	_DELETE,
	_OPTIONS,
	_TRACE
};

enum _Opcode
{
	Continuation = 0,	// ´ÜÆíÈ­
	UTF_8,
	Binary,
	Control_Code1,
	Control_Code2,
	Control_Code3,
	Control_Code4,
	Control_Code5,
	Close,
	Ping,
	Pong
};

// typedef

typedef DWORD(WINAPI *DATAPROCESSPROC)(LPVOID);
typedef unsigned long long QWORD, *LPQWORD;
typedef struct SOCKET_EXTENSION SOCKETEX, *PSOCKETEX;

typedef struct RequestHdr
{
	LPSTR Name;
	LPSTR Value;
	CHAR Alloc;
}ReqHdr, ReqVar, *pReqHdr, *pReqVar;

typedef struct Header
{
	methods HdrMethods;
	LPSTR	RequestURL;
	LPSTR	HTTPVersion;
	pReqHdr RequestHeader;
	DWORD	RequestHeaderCount;
	pReqHdr SendHeader;
	DWORD	SendHeaderCount;
	pReqVar RequestURLQuery;
	DWORD	RequestURLQueryCount;
	BOOLEAN Download;

	LPSTR	HeaderEnd;
	DWORD	ContentRead;
	LPQWORD TotalRead;
	LPQWORD	ContentLength;
}Hdr, *pHdr;

typedef struct SOCKET_EXTENSION
{
	SOCKET		sck;
	IN_ADDR		ip;
	BYTE		Type;
	BOOLEAN		RcvPong;
	pHdr		pHeader;
	LPSTR		Session;
}SOCKETEX, *PSOCKETEX;

typedef struct Send_Header
{
	pReqHdr RequestHeader;
	DWORD RequestHeaderCount;
}sHdr, *psHdr;

typedef struct _wsckHdr
{
	union
	{
		struct
		{
#ifdef __BIG_ENDIAN__
			BOOLEAN		FIN : 1;
			BYTE		RSV : 3;
			BYTE		OpCode : 4;
			BOOLEAN		MASK : 1;
			BYTE		PayloadLength : 7;
#else
			BYTE		OpCode : 4;
			BYTE		RSV : 3;
			BOOLEAN		FIN : 1;
			BYTE		PayloadLength : 7;
			BOOLEAN		MASK : 1;
#endif
		};

		WORD Data;
	};
}sckDataFrame;

typedef struct myfd_set
{
	u_int   fd_count;               /* how many are SET? */
	SOCKET  fd_array[1];			/* an array of SOCKETs */
}myfd_set;

typedef struct MIME_TYPE
{
	CONST CHAR	Extension[10];
	CONST CHAR	Mime[100];
}MIME_TYPE;

#endif