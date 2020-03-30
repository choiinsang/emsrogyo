#ifndef __CMYSERVER__
#define __CMYSERVER__

#include "MyClient.h"
#include "CMyRequest.h"
#include "CMyThread.h"

typedef class CMultiServer<MyClient, CMyRequest, CMyThread> CVanilaServer;

class CMyServer : public CVanilaServer
{
public:
        CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp);
        void TimerFunc();
};

#endif


