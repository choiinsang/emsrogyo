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
	//m_spEmsQueue ó�� ������
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

	        //CMD Header ����
	        AMQPExchange *getCMDExchange() { return m_pamqpExchange; }
	        void          setExHeader   (AMQPExchange* ex, const char* hdrType, const char* hdrStr, bool bFlag=true);
	        void          setExHeader   (AMQPExchange* ex, const char* hdrType, int hdrData, bool bFlag=true);

	        //DB üũ �� ���μ��� ���� �� CMDó��
	        bool  checkPrevRunProcess   ();    //���� Campaign ���� ���μ���
	        void  checkCampaignState    (const char* pCampaignNumber, shared_ptr<CEmsDbThread> &spEmsDbThread); //- Campaign ���μ����� ���� üũ
	        void  checkPauseCampaignList();    //Check Campaign Pause List
	        void  checkTmpCampaignList  (time_t currTime); //m_TmpCampaignList üũ
	        //Send CMD
	        void  sendCMDtoRMQ          (const char* pCmdmsg, const char* route_key=RMQ_ROUTE_KEY_CMD);         //- Rabbitmq ������ CMD �޽��� ����
	        bool  sendCMDCampaignCreate (const char* pCampaignNumber, int iGroupNo, const char* pGroupName);
	        bool  sendCMDCpMailInfo     (shared_ptr<CEmsDbThread> spEmsDbThread, MYSQL_ROW &MySQLRow, const char *pCampaignNumber, const char *pServerName=NULL);
	        void  sendCMDCampaignClose  (const char* pCampaignNumber, int iGroup_no);                           //- �ش� Campaign ���μ����� �����Ŵ - ���� �Ϸ�� ȣ��
	        void  sendCMDCampaignPause  (const char* pCampaignNumber, int iGroup_no);                           //- �ش� Campaign ���μ����� ������Ŵ - ����� ������ ȣ��
	        void  sendCMDServerStop     (const char* pCampaignNumber, const char* pServerName);                 //- �ش� ������ �ش� �ε����� ����(ALL_INDEX�� ���, �ش缭�� �޽��� �Լ� ����)

	        //Listen CMD
	        bool  sendServerConfig      (const char* pServerIP, const char *pServerName);
	        bool  addToServerList       (const char* pServerGroupNo, const char *pServerIP, const char *pServerName);
	        void  procMessage_heartbeat (AMQPMessage* pMsg);  //EmsServer Heartbeat(���� ���� ������Ʈ)
	        void  procMessage_ReqGet    (AMQPMessage* pMsg);
	        bool  createEACMDQueue      ();                   // EmsAgent ���� Listen CMD Queue

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
	        shared_ptr<CEmsDbThread>                          getEmsDbThread          (const char* pCampaignNumber); //m_TmpCampaignList�� �߰�
	        
	        //-�ʿ���� ��...map<double, shared_ptr<stCampaignInfo> >         *getCampaignInfoList() { return &m_CampaignInfoList; }

	        bool  checkGroupProcessCount  (int group_no);
	        void  addCampaignToList       (int nCount);
	        bool  checkProcState          ();
	        void  checkServerHB           (time_t curTime);
	        
	        bool  isCheckTimeOver         (time_t &prevTime, time_t currTime, int timeInterval=MAX_CPLIST_CHECK_WAIT_TIME);
	        void  checkSendCPList         (time_t currTime);           // Send Process List To CMD Queue
	        void  checkSendConnHBCMDQ     (time_t currTime);           // Send CMD Queue Connection HeartBeat
	        void  checkTmpGroupInfo       (time_t currTime);           // Check Unused TmpGroupInfo 

	        void  sendProcessList       (const char *pServerName, const char *pGroup_no);           // Process List�� CMD ť�� ����

	 static CEmsAgentServer *getEmsAgentServer() { return gEmsAgentServer; }

public:
	CEmsMysql    m_EmsMysql;      // Campaign ������ ���� DB����. 
	CEmsMysql    m_EmsMysqlSub;
	CEmsMysql    m_EmsCMDMysql;   // EMS Agent CMD ó�� DB Connection
	

private:
	pthread_t    m_ThreadDBProc;  // Campaign�� �����ϴ� Thread

	unordered_map<string, shared_ptr<CEmsDbThread> >  m_CampainList;      // Campaign���� ó���ϴ� EmsDbThread�� �����Ͽ� ���� <= Group���� Process���� �����ؾ� �Ѵ�.
	unordered_map<string, shared_ptr<CEmsDbThread> >  m_TmpCampaignList;  // �ӽ� Campaign ����Ʈ(�α�ó���� �ѽ������� ����)
	deque<string>                                     m_EndCampainList;   // ������Ѿ��� Campaign List�� ����
	
	shared_ptr<CEmsDbCommonQThread>                   m_spCommonQThread;  // Common Queue Message�� ó���ϱ����� ��ü ������
	unordered_map<int, shared_ptr<stGroupInfo> >      m_GroupInfoList;    // Group ���� ����

	time_t        m_tLastCPTime;
	time_t        m_tLastCMDHBTime;
	char          m_chDBIPAddr[SZ_IP4ADDRESS+1];
	int           m_iDBPort;
	char          m_chDBName  [SZ_NAME+1];
	char          m_chDBUser  [SZ_NAME+1];
	char          m_chDBPasswd[SZ_PASSWD+1];
	int           m_iMaxCampaignCount;          // Campaign �� �ִ� ó�� ���μ��� ����
	
	//EMS CMD Queue Variable
	bool          m_bRMQUsable;                 // Rabbitmq Connection Usable State
	AMQPQueue    *m_EACMDQueue;                 // EmsAgent CMD Listen Queue
	AMQPExchange *m_pamqpExchange;              // Send to EmsServer CMD Send Exchange

	bool          m_bSendProcessList;           // Send Process List Flag To EmsServer
	
	bool          m_bCheckPauseFlag;

	static CEmsAgentServer *gEmsAgentServer;    // CMD Message Queue Event ó���� ���� static ���� �߰�

};


#endif   //__CEMS_WORK_SERVER_HEADER__

