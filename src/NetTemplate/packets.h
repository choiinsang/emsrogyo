#ifndef __PACKETS__
#define __PACKETS__

#include "packet_header.h"

#pragma pack(1)

#define	MSG_CHAT	1000

typedef struct _PACKET_CHAT
{
	PACKET_HEADER ph;
	char chat[256];
} PACKET_CHAT;

#pragma pack()
#endif

