#include "CMyThread.h"

CMyThread::CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue)
: CThread<CMyRequest>(identifier, pMutex, pCond, pQueue)
{
}

void CMyThread::processRequest(shared_ptr<CMyRequest> pRequest)
{
        PACKET_HEADER *pph = (PACKET_HEADER *)pRequest->getPacket().get();

		switch(ntohl(pph->ulType))      
		{
		case MSG_CHAT:
			PACKET_CHAT *pc;
			pc = (PACKET_CHAT *)pph;
			printf("Client %ld said : %s\n", pRequest->getClient()->m_ulKey, pc->chat);
			pRequest->getClient()->Send(pc, ntohl(pph->ulSize));
			break;
		case 11:
			printf("GGGGGGGGG\n");
			//pRequest->setPacket((char*)"Kabcd", 6);
			//pRequest->setClient(pRequest->getClient());
			//pRequest->setPacket((char*)pRequest->getClient()->m_ReadBuffer.get(), 5+1); // +1 '\0' Ãß°¡
			//pRequest->getPacket().get()[5] = '\0';
			//addQueue(pRequest);
			pRequest->getClient()->Send((void*)"Kabcd", 6);
			//pClient->Send((void*)"Kabcd", 6);
		default:
			break;
		}

}

