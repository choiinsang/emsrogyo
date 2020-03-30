//##############################################################################################
#ifndef __CEMS_QUEUE_HEADER__
#define __CEMS_QUEUE_HEADER__

#include <deque>
#include <string.h>

#include "LockObject.h"
#include "EmsCommon.h"
//#include "EmsMX.h"

using namespace boost;
using namespace std;


typedef struct _stCpMailInfo
{
	_stCpMailInfo(){
		_cpIndex         = NULL ;
		_cpSenderName    = NULL ;
		_cpSenderMail    = NULL ;
		_cpMailTitle     = NULL ;
		_cpMailBody      = NULL ;
		_cpSenderDomain  = NULL ;
		_bcpUseWStr      = false;
		_icpMailTitleLen = 0;
		_icpMailBodyLen  = 0;
	};
		
	~_stCpMailInfo()
	{
		try{
		if(_cpIndex        != NULL) free(_cpIndex);
		if(_cpSenderName   != NULL) free(_cpSenderName);
		if(_cpSenderMail   != NULL) free(_cpSenderMail);
		if(_cpMailTitle    != NULL) free(_cpMailTitle);
		if(_cpMailBody     != NULL) free(_cpMailBody);
		if(_cpSenderDomain != NULL) free(_cpSenderDomain);
		}
		catch(...){
		}
	};

	public:
		char * _cpIndex;
		char * _cpSenderName;
		char * _cpSenderMail;
		char * _cpMailTitle;
		char * _cpMailBody;
		char * _cpSenderDomain;
		bool   _bcpUseWStr;	
		int    _icpMailTitleLen;
		int    _icpMailBodyLen;

}stCpMailInfo;


class CCpMail
{
	public:
		            CCpMail();
	             ~CCpMail();
		
		bool        setMailInfo      (const char * hType, const char * hData);
		const char *getMailInfo      (const char * hType);

		void        setWorkerName    (const char * pWorkerName){ m_pWorkerName = pWorkerName;}
		const char *getWorkerName    ()                        { return m_pWorkerName; }

		void        setMode          (int iMode)        { m_iMode = iMode;}
		int         getMode          ()                 { return m_iMode; }

		void        setSmtpCode      (int iSmtpCode)    { m_iSmtpCode = iSmtpCode; }
		int         getSmtpCode      ()                 { return m_iSmtpCode; }

		void        setSmtpStep      (int iSmtpStep)    { m_iSmtpStep = iSmtpStep; }
		int         getSmtpStep      ()                 { return m_iSmtpStep; }
		void        nextSmtpStep     ()                 { m_iSmtpStep++; }

		void        setExpResultCode (int iExpResultCode){ m_iExpResultCode = iExpResultCode; }
		int         getExpResultCode ()                  { return m_iExpResultCode; }

		void        setExpResultStr  (string ExpResultStr){ m_ExpResultStr = ExpResultStr; }
		string      getExpResultStr  ()                   { return m_ExpResultStr; }

		bool        getStepComplete  ()                 { return m_IsStepComplete; }
		void        setStepComplete  (bool bIsComplete) { m_IsStepComplete = bIsComplete; }

		void        setRetryCount    (int iRetryCount)  { m_TryCnt = iRetryCount; }
		int         getRetryCount    ()                 { return m_TryCnt; }
		void        incRetryCount    ()                 { m_TryCnt++; }
		
		void        setTimeStamp     ();
		time_t      getTimeStamp     ()                 { return m_Time; }
		unordered_map<string, char*> *getCpMailInfo ()  { return &m_mapCpMailInfo; }
		
		void        setCpMailState   (bool bState)      { m_bCpMailState = bState; }
		bool        getCpMailState   ()                 { return m_bCpMailState; }
		
		char       *getConnIP        ()                 { return m_connIPAddr; }
		void        setConnIP        (const char *ipaddr);
		
	private:
		const char *m_pWorkerName;   //(Campaign Name) == (Queue Name)
		time_t      m_Time;
		bool        m_bCpMailState;  // CpMail Process Use State Active or Not(Active: true, Inactive:false)
		int         m_iMode;
		int         m_TryCnt;
		int         m_iSmtpCode;
		int         m_iSmtpStep;
		int         m_iExpResultCode;
		string      m_ExpResultStr;
		bool        m_IsStepComplete;
		char        m_connIPAddr[SZ_IP4ADDRESS+1];
		
	private:		
		unordered_map<string, char*> m_mapCpMailInfo;
};

typedef std::deque<shared_ptr<CCpMail> > deqCpMail;

// Queue에 들어가는 메시지 타입을 어떻게 할것인지 고려
// 현재는 구조체 형식의 CpMail 이 들어가고있음=>메일 데이터를 보내야함.
// 'CMD'나 'LOG'를 처리하는 Queue 의 경우 static으로 처리하여도 상관없지만 
// 메일 메시지를 처리하는 Queue의 경우는 각각의 이름으로 생성되어야 하므로 클래스 객체로 남겨둠.
 
class CEmsQueue : public LockObject
{
	public:
		CEmsQueue();
		~CEmsQueue();
		
		int                        addCpMail   (boost::shared_ptr<CCpMail> spCpMail);
		boost::shared_ptr<CCpMail> getCpMail   ();
		int                        getQueueSize() { return m_deqMailQueue.size(); };

	private:
		deqCpMail   m_deqMailQueue;
};

#endif  //__CEMS_QUEUE_HEADER__
