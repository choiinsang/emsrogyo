#ifndef __CEMS_WORKER_MANAGER_HEADER__
#define __CEMS_WORKER_MANAGER_HEADER__


#include "../NetTemplate/CThreadPool.h"
#include "EmsRequest.h"
#include "EmsThread.h"
#include "EmsRMQManager.h"
#include "EmsWorkerThreadPool.h"
#include "EmsWorkServer.h"

//----------------------------------------------------------------------
// Auth : ischoi
// Do   : ThreadPool�� Thread ���� ���� Queue�� �Էµ� ���� ��û�� ó��
// ThreadPool ���� Thread�� ���Ͽ� �����Ͽ� ���� ����ó���� �������� ��:DKIM / SendMail
// EmsQueue(queue�� �޽��� �޾Ƽ� �ֵ����ϰ�, ťũ�� �����ϸ鼭 �޽����� �����������ϴ� ���� �ʿ�
// RMQConnection : 
// Worker�� �����ϴ� ThreadPool�� ������
//----------------------------------------------------------------------
typedef unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > mapWorkersPool;

class CEmsWorkerManager: public LockObject
{
	public:
		virtual     ~CEmsWorkerManager();
		
		static CEmsWorkerManager * getInstance();
		
		void         startThread      ();             //CEmsWorkerManager �����ϱ� ���� ������ ����
		void         setCOMMONQueue   (const char *strGroupRouteKey);
		bool         checkGroupNo     (int iGroupNo);
	
		static int   onCancel         (AMQPMessage * message );
		static int   onMessage        (AMQPMessage * message );
		bool         parseCMDMsg      (AMQPMessage * message);

		//Group ����
		void         bindGroupName    (const char *pGroupName);
		void         createCommonQueue(const char *pchCommonQueueName);
		void         setServerName    (const char *pServerName) { m_pServerName = pServerName; }
		const char  *getServerName    ()            { return m_pServerName; }
		
		bool         isStop           ()            { return m_bIsStop;   }
    void         setStopState     (bool bState) { m_bIsStop = bState; }
		bool         isRun            ()            { return m_bIsRun;    }
		void         setRunState      (bool bState) { m_bIsRun = bState;  }
    
		// CMD Queue Connection
		void         setRMQUsable     (bool bState) { m_bRMQUsable = bState; }
		bool         getRMQUsable     () { return m_bRMQUsable; }

		bool         createCMDQueue   ();             // EmsServer CMD Queue�� ����/����
		bool         connectEACMDQueue();             // EmsAgent CMD Queue ����
		void         resetRMQConn     ();


		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > *getWorkersPoolMap    () { return &m_mapEmsWorkersPool; }
		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > *getWorkersPoolMap_Del() { return &m_mapEmsWorkersPool_Del; }
		
		//Set Send CMD Message Info
		bool         getMsgHeaderInfo  (int iMsgType, unordered_map<string, string> *pMapCmdHeaderInfo, int iGroupNo=0, int iServerNo=0);          // Header Info ����
		bool         setEACMDMsgHeader (unordered_map<string, string> *pInfomap);                                 // CMD �޽��� ��� ����
		void         sendToEACMDMQ     (unordered_map<string, string> *pMsgInfoMap, const char *l_pszFMT, ... );  // CMD �޽��� ���� �Լ�
		// Send CMD To EmsAgent
		bool         sendReqProcList           (int iGroupNo);  // EmsServer ���۽� EmsAgent�� �˸�������  Log Queue�� �޽��� ����
		bool         sendReqServerConfigInfo   ();  // EmsServer  ���� ���� ��û
		bool         sendHeartBeat             ();  // EmsAgent CMD ť�� Heartbeat �޽��� ����
		// Receive CMD
		void         processCreateWorkerThread (const char *qname, int iThreadMode=MODE_M, int nThreads = MAX_WORKERTHREAD_COUNT);
		void         processInitMailCampaign   (const char *qname, AMQPMessage * pMessage);
		void         addToWorkersPool          (const char *qname, int iThreadMode, int nThreads); // ť(Campaign)�̸����� ������ Ǯ ����/�߰�

		void         processDeleteWorkerThread (const char *qname);  // ť(Campaign)�̸����� ������ Ǯ ����/����
		void         processPauseWorkerThread  (const char *qname);  // ť(Campaign)�̸��� ������ �Ͻ����� 
		void         processrestartWorkerThread(const char *qname);  // ť(Campaign)�̸��� ������ ����� 
		void         processServerStop         ();  // ���μ��� ����(qname�� CMD_ALL_INDEX �� ��� ��� ���μ��� ����)
		void         processCheckList          (AMQPMessage *pMessage);
		void         processSetServerConf      (AMQPMessage *pMessage);

		shared_ptr<CEmsWorkerThreadPool> getWorkersPool(const char * qname);

		void         setWorkerServer           (CEmsWorkServer *theServer);
		bool         IsNullWorkerServerClient  ();
		
		//Init DKIM
		void         InitEmsDKIM ();
		unordered_map<std::string, shared_ptr<CEmsDKIM> > *getEmsDKIMList() { return &m_mapCEmsDKIMList; }
		
		//Check Config Thread
		int          checkServerConfigStart();
		static void *ChkThreadFunc         (void *param); //Config ���� Ȯ��

		unordered_map<int, shared_ptr<stGroupInfo> > * getGroupInfos() { return &m_mapGroupInfo; }
		
	private:
		CEmsWorkerManager();

  private:
  	static CEmsWorkerManager * m_pEmsWorkerManager;
  	pthread_t                  m_chkThread;
  	
  	const char     *m_pServerName;
  	unordered_map<int, shared_ptr<stGroupInfo> > m_mapGroupInfo;
  	
  	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> > m_mapEmsWorkersPool;     // ������ Ǯ�� ����
  	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> > m_mapEmsWorkersPool_Del; // ���� ������ Ǯ�� ����
  	unordered_map<std::string, shared_ptr<CEmsDKIM> >             m_mapCEmsDKIMList;       // DKIM ����Ʈ����

  	bool            m_bRMQUsable;      // RMQ Connection State
  	AMQPQueue      *m_CMDQueue;        // EmsServer CMD
  	AMQPExchange   *m_pEACMDExchange;  // EmsAgent  CMD
  	CEmsWorkServer *m_WorkServer;
  	//Thread State
  	bool            m_bIsStop;
  	bool            m_bIsRun;
  	pthread_t       m_EmsWMThread;
  	bool            m_bIsActiveCommonQueue; //COMMON QUEUE is Active or Not

};

#define theWorkerManager() CEmsWorkerManager::getInstance()

#endif   // __CEMS_WORKER_MANAGER_HEADER__

