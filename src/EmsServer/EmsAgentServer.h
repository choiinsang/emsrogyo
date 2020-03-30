#ifndef __CEMS_WORK_SERVER_HEADER__
#define __CEMS_WORK_SERVER_HEADER__

//#ifdef __USETR1LIB__
//#include <tr1/unordered_map>
//#define unordered_map std::tr1::unordered_map
//#else
//#include <boost/unordered_map.hpp>
//#define unordered_map boost::unordered_map
//#endif

#include <deque>

#include "../NetTemplate/CClientEpoll.h"
#include "../NetTemplate/CServerEpoll.h"
#include "EmsDbThread.h"
#include "EmsDbCommonQThread.h"
#include "EmsRequest.h"
#include "EmsClient.h"

#include "sth_syslog.h"

using namespace boost;
using namespace std;

//---------------------------
// Class CEmsAgentServer
//---------------------------
class CEmsAgentServer : public CServer< CEmsClient >, public LockObject
{

public:
	              CEmsAgentServer       ( int iMaxClientReadBuffer
	                                    , int iMaxClientWriteBuffer
	                                    , int iPort, int delay
	                                    , int nMaxClients
	                                    , int timer
	                                    , int nWorkers
	                                    , FILE *fp);
	virtual      ~CEmsAgentServer       ();

	//Campaign Number Change String to BigInt Number
	        //unsigned long getCpNoStrToULong(char * strCampaignNo);
	//m_spEmsQueue 처리 스레드
	        void  checkDB               ();
	        
	        bool  StartDBProcess        ();
	static  void *ThreadFuncDBProc      (void * param);
	
	        int   getCampaignCount      (int group_no=-1); // -1:all, 0 or Over 0:Group_no Count
	        int   getEndCampaignCount   () { return m_EndCampainList.size(); }
	        
	        //Queue Connection State
	        void  setRMQUsable          (bool bState) { m_bRMQUsable = bState; }
	        bool  getRMQUsable          () { return m_bRMQUsable; }
	        void  resetRMQConn          ();
	        void  reconnRMQConn         ();

	        //CMD Header 설정
	        AMQPExchange *getCMDExchange() { return m_pamqpExchange; }
	        void          setExHeader   (AMQPExchange* ex, const char* hdrType, const char* hdrStr, bool bFlag=true);
	        void          setExHeader   (AMQPExchange* ex, const char* hdrType, int hdrData, bool bFlag=true);

	        //DB 체크 및 프로세스 변경 및 CMD처리
	        bool  checkPrevRunProcess   ();    //이전 Campaign 복구 프로세스
	        void  checkCampaignState    (const char* pCampaignNumber, shared_ptr<CEmsDbThread> &spEmsDbThread); //- Campaign 프로세스의 상태 체크
	        void  checkPauseCampaignList();    //Check Campaign Pause List
	        void  checkTmpCampaignList  (time_t currTime); //m_TmpCampaignList 체크
	        //Send CMD
	        void  sendCMDtoRMQ          (const char* pCmdmsg, const char* route_key=RMQ_ROUTE_KEY_CMD);         //- Rabbitmq 서버로 CMD 메시지 전송
	        bool  sendCMDCampaignCreate (const char* pCampaignNumber, int iGroupNo, const char* pGroupName);
	        bool  sendCMDCpMailInfo     (shared_ptr<CEmsDbThread> spEmsDbThread, MYSQL_ROW &MySQLRow, const char *pCampaignNumber, const char *pServerName=NULL);
	        void  sendCMDCampaignClose  (const char* pCampaignNumber, int iGroup_no);                           //- 해당 Campaign 프로세스를 종료시킴 - 서비스 완료시 호출
	        void  sendCMDCampaignPause  (const char* pCampaignNumber, int iGroup_no);                           //- 해당 Campaign 프로세스를 정지시킴 - 사용자 정지시 호출
	        void  sendCMDServerStop     (const char* pCampaignNumber, const char* pServerName);                 //- 해당 서버의 해당 인덱스를 삭제(ALL_INDEX인 경우, 해당서버 메시지 입수 중지)

	        //Listen CMD
	        bool  sendServerConfig      (const char* pServerIP, const char *pServerName);
	        bool  addToServerList       (const char* pServerGroupNo, const char *pServerIP, const char *pServerName);
	        void  procMessage_heartbeat (AMQPMessage* pMsg);  //EmsServer Heartbeat(연결 상태 업데이트)
	        void  procMessage_ReqGet    (AMQPMessage* pMsg);
	        bool  createEACMDQueue      ();                   // EmsAgent 서버 Listen CMD Queue

	 static int   onCancel       (AMQPMessage* message);
	 static int   onMessage      (AMQPMessage* message);

	        void  Init();
	        bool  ConnectDB             (const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort);	
	
	        unordered_map<string, shared_ptr<CEmsDbThread> > *getCampaignList    () { return &m_CampainList; }
	        unordered_map<string, shared_ptr<CEmsDbThread> > *getTmpCampaignList () { return &m_TmpCampaignList; }
	        deque<string>                                    *getEndCampaignList () { return &m_EndCampainList; }
	        unordered_map<int, shared_ptr<stGroupInfo> >     *getGroupInfoList   () { return &m_GroupInfoList; }

					shared_ptr<CEmsDbCommonQThread>                   getspCommonQThread () { return m_spCommonQThread; }

	        shared_ptr<CEmsDbThread>                          getDbNameFromeTmpCpList (const char* pCampaignNumber);
	        shared_ptr<CEmsDbThread>                          getEmsDbThread          (const char* pCampaignNumber); //m_TmpCampaignList에 추가
	        
	        //-필요없는 듯...map<double, shared_ptr<stCampaignInfo> >         *getCampaignInfoList() { return &m_CampaignInfoList; }

	        bool  checkGroupProcessCount  (int group_no);
	        void  addCampaignToList       (int nCount);
	        bool  checkProcState          ();
	        void  checkServerHB           (time_t curTime);
	        
	        bool  isCheckTimeOver         (time_t &prevTime, time_t currTime, int timeInterval=MAX_CPLIST_CHECK_WAIT_TIME);
	        void  checkSendCPList         (time_t currTime);           // Send Process List To CMD Queue
	        void  checkSendConnHBCMDQ     (time_t currTime);           // Send CMD Queue Connection HeartBeat
	        void  checkTmpGroupInfo       (time_t currTime);           // Check Unused TmpGroupInfo 

	        void  sendProcessList       (const char *pServerName, const char *pGroup_no);           // Process List를 CMD 큐에 전송

	 static CEmsAgentServer *getEmsAgentServer() { return gEmsAgentServer; }

public:
	CEmsMysql    m_EmsMysql;      // Campaign 관리를 위한 DB연결. 
	CEmsMysql    m_EmsMysqlSub;
	CEmsMysql    m_EmsCMDMysql;   // EMS Agent CMD 처리 DB Connection
	

private:
	pthread_t    m_ThreadDBProc;  // Campaign을 관리하는 Thread

	unordered_map<string, shared_ptr<CEmsDbThread> >  m_CampainList;      // Campaign별로 처리하는 EmsDbThread를 생성하여 관리 <= Group별로 Process들을 관리해야 한다.
	unordered_map<string, shared_ptr<CEmsDbThread> >  m_TmpCampaignList;  // 임시 Campaign 리스트(로그처리시 한시적으로 관리)
	deque<string>                                     m_EndCampainList;   // 종료시켜야할 Campaign List를 관리
	
	shared_ptr<CEmsDbCommonQThread>                   m_spCommonQThread;  // Common Queue Message를 처리하기위한 객체 쓰레드
	unordered_map<int, shared_ptr<stGroupInfo> >      m_GroupInfoList;    // Group 정보 관리

	time_t        m_tLastCPTime;
	time_t        m_tLastCMDHBTime;
	char          m_chDBIPAddr[SZ_IP4ADDRESS+1];
	int           m_iDBPort;
	char          m_chDBName  [SZ_NAME+1];
	char          m_chDBUser  [SZ_NAME+1];
	char          m_chDBPasswd[SZ_PASSWD+1];
	int           m_iMaxCampaignCount;          // Campaign 의 최대 처리 프로세스 개수
	
	//EMS CMD Queue Variable
	bool          m_bRMQUsable;                 // Rabbitmq Connection Usable State
	AMQPQueue    *m_EACMDQueue;                 // EmsAgent CMD Listen Queue
	AMQPExchange *m_pamqpExchange;              // Send to EmsServer CMD Send Exchange

	bool          m_bSendProcessList;           // Send Process List Flag To EmsServer
	
	bool          m_bCheckPauseFlag;

	static CEmsAgentServer *gEmsAgentServer;    // CMD Message Queue Event 처리를 위해 static 변수 추가

};


#endif   //__CEMS_WORK_SERVER_HEADER__

