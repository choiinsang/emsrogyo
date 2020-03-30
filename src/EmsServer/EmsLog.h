//#pragma once
#ifndef __CEMS_LOG_HEADER__
#define __CEMS_LOG_HEADER__

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdarg>
#include <string.h>

#include "LockObject.h"
#include "DefineNetGlobal.h"
#include "./amqpcpp-master/include/AMQPcpp.h"

#include "EmsCommon.h"
#include "EmsQueue.h"


using namespace boost;

typedef struct _stMsgInfo{
	_stMsgInfo(int iLogType, int iMsgType, shared_ptr<CCpMail> spCpMail, int iMsg, char *pStr=""){
		_iLogType = iLogType;
		_iMsgType = iMsgType;
		_iMsg     = iMsg;
		_strMsg   = string(pStr);
		_spCpMail = spCpMail;
	}
	
	int    _iLogType;
	int    _iMsgType; 
	int    _iMsg;
	string _strMsg;
	shared_ptr<CCpMail> _spCpMail; 
} stMsgInfo;


class CEmsLog : public LockObject
{
// Construction
public:
	virtual ~CEmsLog();
	       
	bool   InitLog         (const char *l_pszPath, const char *pLogName = "EMSLog");
	// CMD Queue Connection
	bool   InitLogQueue    (const char *pQueueName, bool bQueueUse=false);

	void   setRMQUsable    (bool bState) { m_bRMQUsable = bState; }
	bool   getRMQUsable    ()            { return m_bRMQUsable; }
	
	void   resetRMQConn    ();
	void   reconnRMQConn   ();

	void   Write           (const char *l_pszFMT, ... );
	void   WriteLOG        (const char *l_pszData);
	void   WriteToMQ       (int logType, unordered_map<string, string> &l_infomap, const char *l_pszFMT, ... );
	
	void   WriteInfo       (shared_ptr<CCpMail> spCpMail, bool bFromMQ=false);
	void   ReadInfo        (char *keyName, shared_ptr<CEmsQueue> &spEmsQueue);
	void   DeleteInfo      (shared_ptr<CCpMail> spCpMail);
	int    remove_dir      (char *path);
	void   CloseInfo       (char *keyName);
	
	bool   isExistDir      (const char *l_pszPath);

	static CEmsLog *getInstance();
	
	void   getMsgHeaderInfo(int iType, shared_ptr<CCpMail> &spCpMail, unordered_map<string, string> *pMapLogHeaderInfo);
	void   sendMsgToQueue  (int iLogType, int iMsgType, shared_ptr<CCpMail> spCpMail, int iMsg);
	void   sendMsgToQueue  (int iLogType, int iMsgType, shared_ptr<CCpMail> spCpMail, const char *pStr);
	void   procMsgInfoQueue();

	void   setRMQCheckerState(bool bState) { m_bCheckThreadStop = bState; } 
	bool   isStopRMQChecker  ()            { return m_bCheckThreadStop; }
	
	int          checkRMQConnStart();
	static void *chkThreadFunc    (void *param); //RMQ Connection »Æ¿Œ
	
std::deque<shared_ptr<stMsgInfo> > *getMsgInfoQueue() { return &m_MsgInfoQueue; }

private:
	       CEmsLog         ();
	bool   setMsgHeader    (unordered_map<string, string> *pInfomap, char *strTime);


// Implementation
private:
	static CEmsLog *m_pLog;
	FILE           *m_FPLog;
	int             m_nLastDay;
	char            m_szPath  [MAX_PATH + 1];
	bool            m_bUseQueue;
	string          m_strLogName;
	string          m_strMsgQueueName;
	bool            m_bCheckThreadStop;
	pthread_t       m_chkThread;       // RMQ Connection Check Thread;
	bool            m_bRMQUsable;      // RMQ Connection State
	AMQPExchange   *m_pMsgExchange;
	
	std::deque<shared_ptr<stMsgInfo> > m_MsgInfoQueue;
	
	int             m_maxRetryCount; // Mail Retry Send Max Count
	
};

#define theEmsLogger()  CEmsLog::getInstance()


#endif //__CEMS_LOG_HEADER__




