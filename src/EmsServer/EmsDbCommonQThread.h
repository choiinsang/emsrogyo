//##############################################################################################
#ifndef __CEMS_DB_COMMON_QUEUE_THREAD_HEADER__
#define __CEMS_DB_COMMON_QUEUE_THREAD_HEADER__

#include <list>
#include "EmsMysql.h"
#include "EmsRMQManager.h"
#include "EmsStructDefine.h"
#include "EmsQueue.h"


//--------------------------------------------------
// CEmsQueue 관련된 부분을 모두 제거하고 
// map을 이용한 문자열 전달로 변경
//--------------------------------------------------
class CEmsDbCommonQThread
{
public:
	              CEmsDbCommonQThread();
	             ~CEmsDbCommonQThread();
	
	bool          isStop            ()           { return m_bIsStop; };
	void          setRunState       (int iState) { m_iRunState = iState; };
	int           getRunState       ()           { return  m_iRunState ; };
               
	AMQPQueue    *getAMQPQueue      (const char *routeKey); //일단 AMQPQueue를 만들고 시작해야하므로...
	void          deleteAMQPQueue   (const char *routeKey);
	AMQPExchange *getAMQPExchange   (const char *routeKey);
	void          deleteAMQPEx      (const char *routeKey);
	void          deleteQ           (const char *routeKey);
	void          deleteRQMConn     (shared_ptr<CCpMail> spCpMail);

	int           getEnqueueCount   (const char *routeKey=NULL);
	bool          sendToRMQCpMail   (MYSQL_ROW &MySQLRow, shared_ptr<CCpMail> spCpMail, bool bUseWstr, bool bUsenderEmail=false);


public:
	void         StartThread        ();
	static void *ThreadFunc         (void *Param);
	
	void         checkPrevCampaign        ();
	bool         isCompleteCheckTime      (int iPeriod);
	bool         checkCompleteCampaignList();
	int          sendMailMsgProcess       (shared_ptr<CCpMail> spCpMail);
	
	int          sendToCampaignCommonQ();
	int          checkCampaign        ();

	bool         ConnectDB            (const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort);
	void         InitEmsDB            ();
		
	bool         setCampaignStep      (const char *pCampaignNumber, int iCpStep, bool bDelFlag=false);

	shared_ptr<CEmsQueue>                                getspCommonQueue     () { return m_spCommonQueue; }
	unordered_map<string, shared_ptr<vector<string> > > *getDBProcList        () { return &m_DBCampaignList;   }
	unordered_map<string, shared_ptr<stCampaignInfo> >  *getstCampaignInfoList() { return &m_stCampaignInfoList;    }
	//unordered_map<string, shared_ptr<vector<string> > > *getEndCampaignList   () { return &m_EndCampaignList; }
	
	bool                    isCampaignStepUpdate(const char *pCampaignNumber, int iCpStep);
	void                    delCampaignStep     (const char *pCampaignNumber);
	
	void                       setGroupInfoList (unordered_map<int, shared_ptr<stGroupInfo> > *pGroupInfoList) { m_pGroupInfoList = pGroupInfoList; }
	shared_ptr<stGroupInfo>    getspGroupInfo   (int group_no);
	shared_ptr<stCampaignInfo> getspCampaignInfo(const char* pCPNo);
	
	void         setisfullMQ(bool bState) { m_bIsFullMQ = bState; }
	bool         getisfullMQ()            { return m_bIsFullMQ; }

private:
	pthread_t  m_Thread;
	
	//check process time
	time_t     m_chkCompleteTime;
	//Rabbitmq 
	unordered_map<string, AMQPQueue* >    m_amqpQueueList;
	unordered_map<string, AMQPExchange* > m_amqpExchangeList;
	int        m_iMaxRetryCount;

	//Thread Flag
	int        m_iRunState;
	bool       m_bIsStop;   // Thread State
	bool       m_bIsFullMQ;

	// DB 설정 정보
	char       m_chDBIPAddr [SZ_IP4ADDRESS +1];
	char       m_chDBName   [SZ_NAME+1];
	char       m_chDBUser   [SZ_NAME+1];
	char       m_chDBPasswd [SZ_PASSWD+1];
	int        m_iDBPort;

	//DB Connection
	CEmsMysql  m_EmsMysql;      //Ems DB Server Select
	CEmsMysql  m_EmsMysqlSub;   //Ems DB Server Update
	
	//Internal Message Queue
	shared_ptr<CEmsQueue>                               m_spCommonQueue;
	unordered_map<string, shared_ptr<vector<string> > > m_DBCampaignList;     //DBName, Campaign_Numbers
	unordered_map<string, shared_ptr<stCampaignInfo> >  m_stCampaignInfoList; //Campaign_Number, CampaignInfo
	//unordered_map<string, shared_ptr<vector<string> > > m_EndCampaignList;    //완료 처리된 메일
	unordered_map<string, int>                          m_mapCampaignStep;    //메일처리 상태 비교
	unordered_map<int,    shared_ptr<stGroupInfo> >    *m_pGroupInfoList;     // GroupInfo 리스트

};


#endif //__CEMS_DB_COMMON_QUEUE_THREAD_HEADER__

