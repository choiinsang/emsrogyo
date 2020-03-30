//##############################################################################################
#ifndef __CEMS_DB_THREAD_HEADER__
#define __CEMS_DB_THREAD_HEADER__

#include <list>
#include "EmsMysql.h"
#include "EmsRMQManager.h"
#include "EmsStructDefine.h"
#include "EmsQueue.h"

//--------------------------------------------------
// CEmsQueue 관련된 부분을 모두 제거하고 
// map을 이용한 문자열 전달로 변경
//--------------------------------------------------
class CEmsDbThread
{
public:
	              CEmsDbThread    (const char *pCampaign_no);
	              CEmsDbThread    (int iState=TSTATE_PREREADY);
	             ~CEmsDbThread    ();
	
	bool          isStop          ()           { return m_bIsStop; }
	void          setRunState     (int iState) { m_iRunState = iState; }
	int           getRunState     ()           { return  m_iRunState ; }
               
	int           getEnqueueCount        ();
	bool          checkCpMailSendComplete();
	bool          sendToRMQCpMail        (MYSQL_ROW &MySQLRow);

	//DB Info
	char         *getDBName       ()  { return m_chDBName; }
	char         *getMailDBName   ()  { return m_chMailDBName; }
	
	//Worker Thread Info
	char         *getCampaignNoStr()  { return m_CampaignNo; }
	void          setCampaignNoStr(const char *pCampaign_no);
	
	void          setCpMailKey    (const char* cpMailKey);
	char         *getCpMailKey    ()  { return m_CpMailKey; }
	
	//void          setGroupNo      (int groupNo)   { m_iGroupNo = groupNo; }
	void          setGroupNo      (const char * pGroupNo);
	int           getGroupNo      ()  { return m_iGroupNo; }
	const char   *getGroupNoToStr ()  { return m_strGroupNo.c_str(); }

	void          setGroupName    (const char* groupName) { m_strGroupName = groupName; }
	const char   *getGroupName    ()                      { return m_strGroupName.c_str(); }

	bool          getCpStepFlag   ();
	
	//Queue Connection State
	void          setRMQUsable    (bool bState) { m_bRMQUsable = bState; }
	bool          getRMQUsable    () { return m_bRMQUsable; }
	void          resetRMQConn    ();
	void          reconnRMQConn   ();

	
	AMQPQueue    *getQueueConn    () { return m_amqpQueue; }
	AMQPExchange *getExchange     () { return m_amqpExchange; }
	
	void          setUseWstr      (bool bUseWstr) { m_UseWstr = bUseWstr; }

	void         StartThread      ();
	static void *GetDbThreadFunc  (void *Param);	
	
	void         setMailDBName    (const char *pMailDBName);

	bool         ConnectDB        (const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort);
	void         InitEmsDB        ();

	int          GetCpMail        (int rCount);
	void         GetCampaignCpMail();
	
	void         setCpStep        (int stepState);
	int          getCpStep        ()              { return m_CpStep;  }
	
	bool         setCampaignStep  (int iCpStep);
	long         getTimeDiff      (char *timestamp);
	
	void         rollbackDBUpdate ();
	shared_ptr<CEmsQueue> procGetMessagesFromQueue();
	
	time_t       getDBThreadTime  () { return m_tTime; }
	void         setDBThreadTime  (time_t tTime) { m_tTime = tTime; }
	
	void         setDelMailBody   (bool bDelMailBody) { m_bDelMailBody = bDelMailBody; }
	bool         getDelMailBody   () { return m_bDelMailBody; } 

private:
	CEmsDbThread();

private:
	pthread_t     m_Thread;
	bool          m_bRMQUsable;  // Rabbitmq Connection Usable State
	AMQPQueue    *m_amqpQueue;
	AMQPExchange *m_amqpExchange;
	int           m_iRunState;
	bool          m_bIsStop;      // Thread State

	CEmsMysql     m_EmsMysql;
	CEmsMysql     m_EmsMysqlSub;
	int           m_iMaxRetryCount;

	// DB 설정 정보
	char  m_chDBIPAddr [SZ_IP4ADDRESS +1];
	char  m_chDBName   [SZ_NAME+1];
	char  m_chDBUser   [SZ_NAME+1];
	char  m_chDBPasswd [SZ_PASSWD+1];
	int   m_iDBPort;
	
	// Mail 정보 DB 이름
	char  m_chMailDBName[SZ_NAME+1];


	// 메일 기본정보
	char  m_CampaignNo [LEN_Campaign_no+1];
	char  m_CpMailKey  [LEN_CpMailKey+1];
	
	char   m_strQuery  [Qry_StrlenBig +1];
	string m_strGroupName;
	string m_strGroupNo;
	int    m_iGroupNo;
	int    m_CpStep;
	bool   m_bCpStepFlag;
	int    m_IsStoped;
	bool   m_UseWstr;
	int    m_iSCount;
	time_t m_tTime;
	bool   m_iState;
	// 메일 발송 후 'mail_body'삭제 플래그
	bool   m_bDelMailBody;

};

#endif //__CEMS_DB_THREAD_HEADER__

