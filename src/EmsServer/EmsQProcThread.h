#ifndef __CEMS_QUEUE_PROCESS_THREAD_HEADER__
#define __CEMS_QUEUE_PROCESS_THREAD_HEADER__

#include <list>
#include "EmsMysql.h"
#include "EmsRMQManager.h"
#include "EmsStructDefine.h"
#include "EmsAgentServer.h"


//--------------------------------------------------
// Log Queue의 메시지를 처리하기 위함
//--------------------------------------------------
class CEmsQProcThread
{
public:
	             ~CEmsQProcThread ();
	           
	static CEmsQProcThread * getInstance();
	
	bool          isStop              ()           {  return m_bIsStop; }
	void          setRunState         (int iState) { m_iRunState = iState; }
	int           getRunState         ()           { return  m_iRunState ; }
	void          StartThread         ();
	//Log Process Functions
	void          procMessage_debug      (AMQPMessage *pMsg);  //Queue로부터 받은 메시지를 처리(실제 로그 처리)
	void          procMessage_DBQueryProc(AMQPMessage *pMsg);

	void          setOption           (int procOpt)  { m_iQProcOpt = procOpt; }
	
	static void  *msgProcThreadFunc   (void *Param);	//Message Process Function
	static int    onMessage           (AMQPMessage * message );
	static int    onCancel            (AMQPMessage * message );
	void          setLogProcessEvent  ();
	//void         procDBQuery         (AMQPMessage *pMsg, int iDBQuery);
                                  
	bool          InitProc            ();
	bool          ConnectDB           ( const char *l_pszHost
	                                 , const char *l_pszUser
	                                 , const char *l_pszPasswd
	                                 , const char *l_pszDBName
	                                 , unsigned int l_iPort);
 
	bool          connProcQueue       ();   // Create Queue Connection
	bool          reConnProcQueue     ();   // RMQ 재연결
	bool          setLogSender        ();   // 프로세스 Log Sender 동작 활성화
	bool          setLogProcess       ();   // 프로세스 Log Queue 메시지 처리 동작 활성화
	int           getEnqueueCount     ();   // Check Queue is Empty or not

	void          setQueueConn       (AMQPQueue * pQCnn) { m_amqpQueue = pQCnn; }
	AMQPQueue    *getQueueConn       () { return m_amqpQueue; }
	AMQPExchange *getExchange        () { return m_amqpExchange; }
	AMQPMessage  *getMessage         ();
	                                 
	void             setEmsAgentServer(CEmsAgentServer *pEmsAgentServer) { m_pEmsAgentServer = pEmsAgentServer; }
	CEmsAgentServer *getEmsAgentServer() { return m_pEmsAgentServer; }
	//bool          sendProcessList    (const char *pServerName);
	
	char         *getDBName          () { return m_chDBName; }
	const char   *getMailDBNameFromCpNo(const char *pCpNo, int iCpMode);
	const char   *getMailDBName      (const char *pCpNo, int iCpMode);
	
	int           getTaxGroupNo      () { return m_iTaxGroupNo;}
		

private:
	CEmsQProcThread ();

private:
	static CEmsQProcThread *m_pEmsQProcThread;
//---------------------------------------------------------
	string        m_strQName;    //Log Queue를 연결 처리하기위한 Queue 이름
//---------------------------------------------------------
	pthread_t     m_QMsgProcThread;
	bool          m_bIsStop;      // Thread State
	int           m_iQProcOpt;
	int           m_iRunState;

	AMQPQueue    *m_amqpQueue;
	AMQPExchange *m_amqpExchange;

	//DB 연결관리
	char          m_chDBIPAddr[SZ_IP4ADDRESS+1];
	int           m_iDBPort;
	char          m_chDBName  [SZ_NAME+1];
	char          m_chDBUser  [SZ_NAME+1];
	char          m_chDBPasswd[SZ_PASSWD+1];
	
	//Group 분기 처리
	int           m_iTaxGroupNo;
	//CEmsMysql     m_EmsMysql;
	CEmsMysql     m_EmsMysqlSub;
	
	//EmsAgentServer 접근을 위한 Pointer
	CEmsAgentServer *m_pEmsAgentServer;
	//Campaign 관리
	unordered_map<string, shared_ptr<CEmsDbThread> >   *m_pCampainList;
	unordered_map<string, shared_ptr<stCampaignInfo> > *m_pstCampaignInfoList; //Campaign_Number, CampaignInfo

};

#define theEmsQProcThread() CEmsQProcThread::getInstance()

#endif //__CEMS_QUEUE_PROCESS_THREAD_HEADER__

