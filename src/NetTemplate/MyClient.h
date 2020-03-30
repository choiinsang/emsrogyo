#ifndef __MYCLIENT__
#define __MYCLIENT__

#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClientEpoll.h"
#include "CMultiServerEpoll.h"
#include "CRequest.h"
using namespace std;

#include "CRemoveList.h"
#include <boost/shared_ptr.hpp>
using namespace boost;

// client

class MyClient : public CClientEpoll
{
public:
        MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll, shared_ptr<CRemoveList <unsigned long> > pRemoveList);
        ~MyClient() {};
};

#endif

