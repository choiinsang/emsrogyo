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
// Do : ThreadPool�� Thread ���� ���� Queue�� �Էµ� ���� ��û�� ó��
// ThreadPool ���� Thread�� ���Ͽ� �����Ͽ� ���� ����ó���� �������� ��:DKIM / SendMail
// EmsQueue(queue�� �޽��� �޾Ƽ� �ֵ����ϰ�, ťũ�� �����ϸ鼭 �޽����� �����������ϴ� ���� �ʿ�
// RMQConnection : EmsWorkerManager�κ��� RMQ �޽����� ���� �޾� ���� ���� ����
// Worker�� �����ϴ� ThreadPool�� ������
// WorkerThreadPool�� ������� ����� �ʰ� ThreadPool�� ���
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

		void         startThread      ();                              //CEmsWorkerManager �����ϱ� ���� ������ ����
		bool         checkMailInfo    () { return m_bCheckMailInfo; }
		bool         setCpMailInfo    (AMQPMessage * message);         //Mail�� Campaign ������ ����
		//Group Number 
		void         setGroupNo       (int iGroupNo) { m_iGroupNo = iGroupNo; }
		int          getGroupNo       () { return m_iGroupNo; }
		//Thread Mode: MODE_I(0:Individual Mail Process) / MODE_M(1:Mass Mail Process)
		int          getThreadMode    () { return m_ThreadMode; }
		//Thread Process
		static void *threadProc       (void *param);       //MQ �޽��� ó�� ������ �Լ�
		int          threadProcess    ();                  //Mail �޽��� ó��
		//Thread Msg Process
		static void *threadProcMsg    (void *param);       //DomainQueue �޽��� ó�� ������ �Լ�
		int          threadProcessMsg (char *pDomainName); //Mail �޽��� ó��

		void         timeUpdate       ();
		bool         isWaitTimeOver   (int iMaxWaitTime);
		void         uncompleteWorkCheck();                //Campaign ��ó�� �޽��� üũ

    bool         isStop           ()            { return m_bIsStop; }
		void         setStopState     (bool bState) { m_bIsStop = bState; }
		bool         isRun            ()            { return m_bIsRun; }
		void         setRunState      (bool bState);
                                  
		char        *getWorkerName    ()            { return m_chWTPName;}
		bool         setEmsQueue      (shared_ptr<CEmsQueue> workServerEmsQueue);
		bool         checkWorkServerQ (int maxMsgCount);
		void         addToWorkServerQ (boost::shared_ptr<CCpMail> spCpMail);
		bool         checkCMDQ        ();                  // Rabbitmq CMD Queue�� ó���ؾ��� ��û�� �ִ��� üũ
		int          processCMD       (string cmdMsg);     // cmdMsg �� ó���ϴ� ���μ��� �Լ�(�ӽ÷� string���� type�� �����Ͽ����� ���� ���� ����(�ڵ� �ѹ���...))
		int          procCMDPraser    ();

		//DKIM
		void                 setDKIMList(unordered_map<std::string, shared_ptr<CEmsDKIM> > *pCEmsDKIMList) { m_pEmsDKIMList = pCEmsDKIMList; };
		shared_ptr<CEmsDKIM> getEmsDKIM (string domainName);

		bool         connAMQPQueue    (const char *qname); // ť(Campaign)�̸����� AMQPQueue ����
		void         setRMQExchange   (int iThreadMode);   // RMQ Exchange ����
		const char  *getRMQExchange   () { return m_pRMQExchange; }
		
		void         pause            ();                  // ť(Campaign) ó�� ���μ����� ����(���) ��Ų��.
		void         restart          ();                  // ť(Campaign) ó�� ���μ����� �ٽ� �����ϵ��� �Ѵ�.

		void         procRequest      (shared_ptr<CEmsRequest> pRequest);

		//Domain Queue List
		void                addToDomainQueueList  (shared_ptr<CCpMail> spCpMail);
		shared_ptr<CCpMail> getFromDomainQueueList();

		void                setLock               ()  { m_lock.Lock();}
		void                unsetLock             ()  { m_lock.Unlock();}
	private:

		char                    *m_chWTPName;      // WorkerThreadPool Name
		shared_ptr<CEmsQueue>    m_spEmsQueue;     // ThreadPool�� Thread ���� �����ϰ� �� Queue : Rabbitmq �޽����� CEmsRequest Ÿ������ �����Ͽ� ť�� ����
		AMQPQueue               *m_pAMQPMsgQueue;  // Mail Message�� ����ִ� RabbitMQ ť ����
		const char              *m_pRMQExchange;   // RMQ_EXCHANGE ����
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

