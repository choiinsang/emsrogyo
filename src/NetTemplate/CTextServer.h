#ifndef __CTEXTSERVER__
#define __CTEXTSERVER__

#include "CServer.h"
#include "boost/shared_array.hpp"
#ifdef WIN32
#pragma warning(disable:4786)
#endif

using namespace boost;

template <typename C> class CTextServer : public CServer<C>
{
public:
	CTextServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	virtual ~CTextServer();
	bool Parser( shared_ptr< C > pClient );
	virtual void ParserMain(shared_ptr< C > pClient, int nLength) { };
	int FindCRLF(shared_array<char> buffer, int nLength);
};

template <typename C>
CTextServer<C>::CTextServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
: CServer<C>(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
}

template <typename C>
CTextServer<C>::~CTextServer()
{
}

template <typename C>
int CTextServer<C>::FindCRLF(shared_array<char> buffer, int nLength)
{
	int cnt = 0;
	while( cnt + 1 < nLength )		// CRLF = 2bytes
	{
		if( '\r' == buffer[cnt] && '\n' == buffer[cnt+1] ) break;
		cnt++;
	}
	if( cnt + 1 < nLength )
		return cnt;
	else
		return -1;
}

template <typename C>
bool CTextServer<C>::Parser(shared_ptr< C > pClient)
{
	if( pClient->m_iReadBuffer <= 0 )
		return false;

	long lSize = FindCRLF(pClient->m_ReadBuffer, pClient->m_iReadBuffer);

	printf("DDDDDDDD 001\n");

	while( -1 != lSize )
	{
		if( pClient->m_bRemove ) 
			break; // 클라이언트 처리 상태 변경 시 중지		

		pClient->m_ReadBuffer[lSize] = '\0';

//		printf("DDDDDDDD  %s\n", pClient->m_ReadBuffer);
		printf("DDDDDDDD 002\n");

		ParserMain(pClient, lSize);
		pClient->m_iReadBuffer -= lSize + 2;		// CRLF = 2		

		if( pClient->m_iReadBuffer > 0 ) 
		{
			memcpy(pClient->m_ReadBuffer.get(), pClient->m_ReadBuffer.get()+lSize+2, pClient->m_iReadBuffer);
		}

		if( pClient->m_iReadBuffer < 0 ) 
		{
			pClient->m_iReadBuffer = 0;
		}
		lSize = FindCRLF(pClient->m_ReadBuffer, pClient->m_iReadBuffer);
	}

	printf("DDDDDDDD 003\n");

	return true;
}

#endif

