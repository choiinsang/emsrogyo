#include "MyClient.h"

MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
: CClientEpoll(ulKey, iMaxReadBuffer, iMaxWriteBuffer, epoll_fd, pRemoveList)
{
        printf("%ld\n", m_ulKey);
}

