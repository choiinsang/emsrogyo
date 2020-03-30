#ifndef __CEMS_QUEUE_PROCESS_THREAD_HEADER__
#define __CEMS_QUEUE_PROCESS_THREAD_HEADER__

#include <list>
#include "EmsMysql.h"
#include "EmsRMQManager.h"
#include "EmsStructDefine.h"
#include "EmsAgentServer.h"


//--------------------------------------------------
// Log Queue�� �޽����� ó���ϱ� ����
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
	void          procMessage_debug      (AMQPMessage *pMsg);  //Queue�κ��� ���� �޽����� ó��(���� �α� ó��)
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
	bool          reConnProcQueue     ();   // RMQ �翬��
	bool          setLogSender        ();   // ���μ��� Log Sender ���� Ȱ��ȭ
	bool          setLogProcess       ();   // ���μ��� Log Queue �޽��� ó�� ���� Ȱ��ȭ
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
	string        m_strQName;    //Log Queue�� ���� ó���ϱ����� Queue �̸�
//---------------------------------------------------------
	pthread_t     m_QMsgProcThread;
	bool          m_bIsStop;      // Thread State
	int           m_iQProcOpt;
	int           m_iRunState;

	AMQPQueue    *m_amqpQueue;
	AMQPExchange *m_amqpExchange;

	//DB �������
	char          m_chDBIPAddr[SZ_IP4ADDRESS+1];
	int           m_iDBPort;
	char          m_chDBName  [SZ_NAME+1];
	char          m_chDBUser  [SZ_NAME+1];
	char          m_chDBPasswd[SZ_PASSWD+1];
	
	//Group �б� ó��
	int           m_iTaxGroupNo;
	//CEmsMysql     m_EmsMysql;
	CEmsMysql     m_EmsMysqlSub;
	
	//EmsAgentServer ������ ���� Pointer
	CEmsAgentServer *m_pEmsAgentServer;
	//Campaign ����
	unordered_map<string, shared_ptr<CEmsDbThread> >   *m_pCampainList;
	unordered_map<string, shared_ptr<stCampaignInfo> > *m_pstCampaignInfoList; //Campaign_Number, CampaignInfo

};

#define theEmsQProcThread() CEmsQProcThread::getInstance()

#endif //__CEMS_QUEUE_PROCESS_THREAD_HEADER__

