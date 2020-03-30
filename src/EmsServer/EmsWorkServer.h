#ifndef __CEMS_WORK_SERVER_HEADER__
#define __CEMS_WORK_SERVER_HEADER__

#include "../NetTemplate/CClientEpoll.h"
#include "../NetTemplate/CServerEpoll.h"
#include "EmsRequest.h"
#include "EmsWorkerThreadPool.h"

#include "sth_syslog.h"

using namespace boost;

//template <typename C, typename R, typename T>	// Client, Request, Thread
//class CEmsWorkServer : public CServer< C >
class CEmsWorkServer : public CServer< CEmsClient >
{

public:
	               CEmsWorkServer (int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	virtual       ~CEmsWorkServer () ;
	        void   ParserMain     ( shared_ptr< CEmsClient > pClient, unsigned long _ulSize);
	        bool   IsNullClientCount();

	        void   connectFail    ( shared_ptr< CEmsClient > pClient, int iState );  //Connect Fail
					void   sendBufFail    ( shared_ptr< CEmsClient > pClient, int iState );  //Send Buffer Data Fail
					void   connectError   ( shared_ptr< CEmsClient > pClient, int iState );  //Server Connection Error Occurred
	
	        bool   setWorkersPool (unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > * pmapWorkersPool);
	        bool   relayEmsRequest(shared_ptr<CEmsRequest> pRequest);
	        void   bindQueue      (const char * qname);
	        int    getRetryCount  () { return m_iRetryCount; };
	        int    getRetryPeriod () { return m_iRetryPeriod;};
	        
	//m_spEmsQueue 처리 스레드
	        bool  StartProcessQueue  ();
	        void  errorProc          (shared_ptr<CCpMail> spCpMail, int errCode, const char *errStr, int maxRetryCount);
	        void  enqueueMsg         (shared_ptr<CCpMail> spCpMail);
	        
	virtual void TimerFunc();

	        
	static  void *ThreadFuncProcQueue(void * param);
	
private:
	pthread_t  m_ThreadProcQueue;

	time_t     m_Time;
	int        m_iRetryCount;
	int        m_iRetryPeriod;
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > * m_pmapEmsWorkersPool;  // WorkerThreadPoolList
	shared_ptr<CEmsQueue> m_spEmsQueue; //WorkerThreadPool 공통 Rabbitmq 메시지 요청 Queue
	int        m_iIncForTimer;
};



#endif   //__CEMS_WORK_SERVER_HEADER__

