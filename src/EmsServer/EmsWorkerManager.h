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
// Do   : ThreadPool의 Thread 들이 공유 Queue에 입력된 메일 요청을 처리
// ThreadPool 안의 Thread는 메일에 서명하여 메일 전송처리를 전담으로 함:DKIM / SendMail
// EmsQueue(queue에 메시지 받아서 넣도록하고, 큐크기 관리하면서 메시지를 가져오도록하는 로직 필요
// RMQConnection : 
// Worker를 포함하는 ThreadPool을 관리함
//----------------------------------------------------------------------
typedef unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > mapWorkersPool;

class CEmsWorkerManager: public LockObject
{
	public:
		virtual     ~CEmsWorkerManager();
		
		static CEmsWorkerManager * getInstance();
		
		void         startThread      ();             //CEmsWorkerManager 관리하기 위한 스레드 시작
		void         setCOMMONQueue   (const char *strGroupRouteKey);
		bool         checkGroupNo     (int iGroupNo);
	
		static int   onCancel         (AMQPMessage * message );
		static int   onMessage        (AMQPMessage * message );
		bool         parseCMDMsg      (AMQPMessage * message);

		//Group 설정
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

		bool         createCMDQueue   ();             // EmsServer CMD Queue를 설정/연결
		bool         connectEACMDQueue();             // EmsAgent CMD Queue 연결
		void         resetRMQConn     ();


		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > *getWorkersPoolMap    () { return &m_mapEmsWorkersPool; }
		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > *getWorkersPoolMap_Del() { return &m_mapEmsWorkersPool_Del; }
		
		//Set Send CMD Message Info
		bool         getMsgHeaderInfo  (int iMsgType, unordered_map<string, string> *pMapCmdHeaderInfo, int iGroupNo=0, int iServerNo=0);          // Header Info 수집
		bool         setEACMDMsgHeader (unordered_map<string, string> *pInfomap);                                 // CMD 메시지 헤더 삽입
		void         sendToEACMDMQ     (unordered_map<string, string> *pMsgInfoMap, const char *l_pszFMT, ... );  // CMD 메시지 전송 함수
		// Send CMD To EmsAgent
		bool         sendReqProcList           (int iGroupNo);  // EmsServer 시작시 EmsAgent에 알리기위해  Log Queue에 메시지 전달
		bool         sendReqServerConfigInfo   ();  // EmsServer  설정 정보 요청
		bool         sendHeartBeat             ();  // EmsAgent CMD 큐에 Heartbeat 메시지 전송
		// Receive CMD
		void         processCreateWorkerThread (const char *qname, int iThreadMode=MODE_M, int nThreads = MAX_WORKERTHREAD_COUNT);
		void         processInitMailCampaign   (const char *qname, AMQPMessage * pMessage);
		void         addToWorkersPool          (const char *qname, int iThreadMode, int nThreads); // 큐(Campaign)이름으로 스레드 풀 생성/추가

		void         processDeleteWorkerThread (const char *qname);  // 큐(Campaign)이름으로 스레드 풀 삭제/제거
		void         processPauseWorkerThread  (const char *qname);  // 큐(Campaign)이름의 스레드 일시정지 
		void         processrestartWorkerThread(const char *qname);  // 큐(Campaign)이름의 스레드 재시작 
		void         processServerStop         ();  // 프로세스 정지(qname이 CMD_ALL_INDEX 인 경우 모든 프로세스 정지)
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
		static void *ChkThreadFunc         (void *param); //Config 설정 확인

		unordered_map<int, shared_ptr<stGroupInfo> > * getGroupInfos() { return &m_mapGroupInfo; }
		
	private:
		CEmsWorkerManager();

  private:
  	static CEmsWorkerManager * m_pEmsWorkerManager;
  	pthread_t                  m_chkThread;
  	
  	const char     *m_pServerName;
  	unordered_map<int, shared_ptr<stGroupInfo> > m_mapGroupInfo;
  	
  	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> > m_mapEmsWorkersPool;     // 스레드 풀을 관리
  	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> > m_mapEmsWorkersPool_Del; // 종료 스레드 풀을 관리
  	unordered_map<std::string, shared_ptr<CEmsDKIM> >             m_mapCEmsDKIMList;       // DKIM 리스트정보

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

