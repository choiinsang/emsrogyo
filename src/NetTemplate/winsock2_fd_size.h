#ifndef __WINSOCK2_FD_SIZE__
#define __WINSOCK2_FD_SIZE__

#ifdef	FD_SETSIZE
#undef	FD_SETSIZE
#endif

#define	FD_SETSIZE	10240	// 10K Users

#include <winsock2.h>
#pragma comment (lib,"ws2_32.lib")

#endif
