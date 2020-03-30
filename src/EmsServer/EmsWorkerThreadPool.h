#ifndef __CEMS_WORKER_THREADPOOL_HEADER__
#define __CEMS_WORKER_THREADPOOL_HEADER__

#include "../NetTemplate/CThreadPool.h"
#include "EmsRequest.h"
#include "EmsQueue.h"
#include "EmsThread.h"
#include "EmsRMQManager.h"
#include "EmsDKIM.h"

//----------------------------------------------------------------------
// Auth : ischoi
// Do : ThreadPool의 Thread 들이 공유 Queue에 입력된 메일 요청을 처리
// ThreadPool 안의 Thread는 메일에 서명하여 메일 전송처리를 전담으로 함:DKIM / SendMail
// EmsQueue(queue에 메시지 받아서 넣도록하고, 큐크기 관리하면서 메시지를 가져오도록하는 로직 필요
// RMQConnection : EmsWorkerManager로부터 RMQ 메시지를 전달 받아 연결 관리 유지
// Worker를 포함하는 ThreadPool을 관리함
// WorkerThreadPool은 스레드로 운영되지 않고 ThreadPool을 운영함
//----------------------------------------------------------------------
class CEmsThread;

class CEmsLock : public LockObject
{
	public:
		CEmsLock(){};
		~CEmsLock(){};
};

class CEmsWorkerThreadPool : public CThreadPool<CEmsRequest, CEmsThread>
{
	public:
		             CEmsWorkerThreadPool (const char *pwtpName, unordered_map<std::string, shared_ptr<CEmsDKIM> > *pEmsDKIMList, int iThreadMode=MODE_M, int nThreads=MAX_WORKERTHREAD_COUNT);
		virtual     ~CEmsWorkerThreadPool ();

		void         startThread      ();                              //CEmsWorkerManager 관리하기 위한 스레드 시작
		bool         checkMailInfo    () { return m_bCheckMailInfo; }
		bool         setCpMailInfo    (AMQPMessage * message);         //Mail의 Campaign 정보를 셋팅
		//Group Number 
		void         setGroupNo       (int iGroupNo) { m_iGroupNo = iGroupNo; }
		int          getGroupNo       () { return m_iGroupNo; }
		//Thread Mode: MODE_I(0:Individual Mail Process) / MODE_M(1:Mass Mail Process)
		int          getThreadMode    () { return m_ThreadMode; }
		//Thread Process
		static void *threadProc       (void *param);       //MQ 메시지 처리 스레드 함수
		int          threadProcess    ();                  //Mail 메시지 처리
		//Thread Msg Process
		static void *threadProcMsg    (void *param);       //DomainQueue 메시지 처리 스레드 함수
		int          threadProcessMsg (char *pDomainName); //Mail 메시지 처리

		void         timeUpdate       ();
		bool         isWaitTimeOver   (int iMaxWaitTime);
		void         uncompleteWorkCheck();                //Campaign 미처리 메시지 체크

    bool         isStop           ()            { return m_bIsStop; }
		void         setStopState     (bool bState) { m_bIsStop = bState; }
		bool         isRun            ()            { return m_bIsRun; }
		void         setRunState      (bool bState);
                                  
		char        *getWorkerName    ()            { return m_chWTPName;}
		bool         setEmsQueue      (shared_ptr<CEmsQueue> workServerEmsQueue);
		bool         checkWorkServerQ (int maxMsgCount);
		void         addToWorkServerQ (boost::shared_ptr<CCpMail> spCpMail);
		bool         checkCMDQ        ();                  // Rabbitmq CMD Queue에 처리해야할 요청이 있는지 체크
		int          processCMD       (string cmdMsg);     // cmdMsg 를 처리하는 프로세스 함수(임시로 string으로 type을 설정하였으나 차후 변경 예정(코드 넘버등...))
		int          procCMDPraser    ();

		//DKIM
		void                 setDKIMList(unordered_map<std::string, shared_ptr<CEmsDKIM> > *pCEmsDKIMList) { m_pEmsDKIMList = pCEmsDKIMList; };
		shared_ptr<CEmsDKIM> getEmsDKIM (string domainName);

		bool         connAMQPQueue    (const char *qname); // 큐(Campaign)이름으로 AMQPQueue 연결
		void         setRMQExchange   (int iThreadMode);   // RMQ Exchange 설정
		const char  *getRMQExchange   () { return m_pRMQExchange; }
		
		void         pause            ();                  // 큐(Campaign) 처리 프로세스를 정지(대기) 시킨다.
		void         restart          ();                  // 큐(Campaign) 처리 프로세스를 다시 시작하도록 한다.

		void         procRequest      (shared_ptr<CEmsRequest> pRequest);

		//Domain Queue List
		void                addToDomainQueueList  (shared_ptr<CCpMail> spCpMail);
		shared_ptr<CCpMail> getFromDomainQueueList();

		void                setLock               ()  { m_lock.Lock();}
		void                unsetLock             ()  { m_lock.Unlock();}
	private:

		char                    *m_chWTPName;      // WorkerThreadPool Name
		shared_ptr<CEmsQueue>    m_spEmsQueue;     // ThreadPool의 Thread 들이 공유하게 될 Queue : Rabbitmq 메시지를 CEmsRequest 타입으로 변경하여 큐에 저장
		AMQPQueue               *m_pAMQPMsgQueue;  // Mail Message가 들어있는 RabbitMQ 큐 연결
		const char              *m_pRMQExchange;   // RMQ_EXCHANGE 설정
		int                      m_iGroupNo;       // Group Number

		//Work Time State
		time_t                   m_tLastWorkTime;

		//Thread State
		pthread_t                m_EmsWMTPThread;
		pthread_t                m_EmsMsgWorkerThread;
		bool                     m_bIsStop;
		bool                     m_bIsRun;
		bool                     m_bIsPause;
		int                      m_ThreadMode;
		int                      m_iThreadCount;

		//Mail Campaign Message
		bool                     m_bCheckMailInfo;
		shared_ptr<stCpMailInfo> m_cpMailInfo;

		//Ems DKIM
		unordered_map<std::string, shared_ptr<CEmsDKIM> > *m_pEmsDKIMList;
			
		//Domain Map Message List
		int                      m_iDomainQueueMsgCount;
		unordered_map<std::string, shared_ptr<CEmsQueue> > m_mapDomainQueueList;

		CEmsLock                 m_lock;
};


#endif   // __CEMS_WORKER_THREADPOOL_HEADER__

