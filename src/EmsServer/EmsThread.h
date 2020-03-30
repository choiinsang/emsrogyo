#ifndef __CEMS_THREAD_HEADER__
#define __CEMS_THREAD_HEADER__

#include "CClientEpoll.h"
#include "EmsRequest.h"
#include "EmsQueue.h"
#include "EmsStructDefine.h"
#include "EmsDKIM.h"
#include "EmsWorkerThreadPool.h"

//------------------------------------------
// Queue의 상태를 체크하여
// 메일을 해당 MX의 서버에 전송한다.
// - Queue Check : Queue에서 메일을 읽어와서 필요 작업(dkim처리)하여 
// - Mail Send   : MX 서버에 메일 전송
//------------------------------------------

class CEmsWorkerThreadPool;

class CEmsThread : public CThread <CEmsRequest>
{
	
public:
	          CEmsThread     (unsigned long  identifier
	                         , shared_ptr< pthread_mutex_t > pMutex
	                         , shared_ptr< pthread_cond_t > pCond
	                         , shared_ptr< queue< shared_ptr< CEmsRequest > > > pQueue);
	
	virtual  ~CEmsThread     ();

	//Thread Mode :  Group:0 / Personal:1 
	int       getThreadMode  ()                { return m_ThreadMode; }
	void      setThreadMode  (int iThreadMode) { m_ThreadMode = iThreadMode; }
	
	void      setThreadPool  (CEmsWorkerThreadPool *pEmsWorkerThreadPool)  { m_pEmsWorkerThreadPool = pEmsWorkerThreadPool; }
	void      enqueueMsg     (shared_ptr<CCpMail> spCpMail);
	
	int       First3ChToNum  (char *_pFirst3Ch);
	int       Check_SmtpCode (shared_ptr<CEmsRequest> _spRequest, char *_pFirst3Ch);
            
	void      setCpMailInfo  (shared_ptr<stCpMailInfo> spCpInfo); // MODE_M : Init Buffer & Set Campaign Mail Informations : Mass Email Send Mode
	void      InitThreadBuf  ();                                  // MODE_I : Init Buffer Individual Email Send Mode
	          
	int       addToBufferLine(char *pHeaderBuf, char * pMsgStr);
	void      enqRetryProc   (shared_ptr<CCpMail> spCpMail, shared_ptr<CEmsClient> spClient);
	          
	void      setPause       ();
	void      setRestart     ();
	void      setRunState    (int iState) { m_iRunState = iState; }
	int       getRunState    ()           { return m_iRunState; }
	
	//Mail Message Porcess(Wide String PreProcess)
	void      preDataProcess ( shared_ptr<CCpMail>      spCpMail
	                         , shared_ptr<stSplitInfo> &spSplitInfoTT
	                         , int                     &iSplitCntTT
	                         , shared_ptr<stSplitInfo> & pSplitInfoBody
	                         , int                     &iSplitCntBody);
	                                         
	shared_ptr<stSplitInfo> getWideString  (shared_ptr<CCpMail> spCpMail, int &ret);

	void      doWideStrProc  ( char                   *pWSBuf
	                         , shared_ptr<CCpMail>     spCpMail
	                         , shared_ptr<stSplitInfo> pMsgSplitInfo
	                         , int                     iSplitInfoCount
	                         , shared_ptr<stSplitInfo> spWStrSplitInfo
	                         , int                     iWSCount
	                         , int                     spSwitch);
	
	//Set/Get EmsDKIM
	void                 setEmsDKIMList (unordered_map<std::string, shared_ptr<CEmsDKIM> > *pEmsDKIMList){ m_pEmsDKIMList = pEmsDKIMList;  };
	shared_ptr<CEmsDKIM> getEmsDKIM     (const char *domainName);
	
	//Set Index Data
	void      setIndexDate   (char *pIdxTimeBuffer, struct tm *ptTime, char *msgKey, int incNum);
	
	//Add to Send Buffer
	int       addMsgToBuf    (char * pSendMsgBuf, const char *pMsg, bool bCRLF=true);
	
	//Send to RMQ Log Message
	bool      sendUpdateLogMsg();
	
	//CThread 상속 구현
	void      processRequest (shared_ptr<CEmsRequest> pRequest);
	bool      preprocess     ();
	
	//Mail Character Type
	const char *getCharSet   () { return m_strCharSet.c_str(); }

private:
	shared_ptr<stCpMailInfo> getCpMailInfo (); // Get Campaign Mail Informations
	bool      resetBuffer    (shared_ptr<CCpMail> spCpMail, int iMode);

private:
	
	//shared_ptr<CEmsQueue> m_spEmsQueue;  //EmsWorkServer->m_spEmsQueue
	CEmsWorkerThreadPool *m_pEmsWorkerThreadPool;  //EmsWorkerThreadPool->EmsThread
	int       m_ThreadMode;              // 0:그룹 메일 프로세스, 1:개인 메일 프로세스
//-----------------------------------------
	char     *m_pThreadBuf;
	int       m_ThreadBufLen;

	char     *m_pEncodeBufTT;
	int       m_EncodeBufLenTT;
           
	char     *m_pTmpBuf;
	int       m_TmpBufLen;
           
	char     *m_pEncodeBufBody;
	int       m_EncodeBufLenBody;
	         
	char     *m_pWStrBuf;
	int       m_WStrBufLen;
           
	char      m_strMonthName[80];
	char      m_strWeekName [80];
	
	char      m_strQuitBuf[SZ_TINYBUFFER];
	
	//Wide Position struct
	int       m_SplitCntTT;	    //Title
	int       m_SplitCntBody; 	//Body

	shared_ptr<stSplitInfo> m_stSplitInfoTT;
	shared_ptr<stSplitInfo> m_stSplitInfoBody;
	
	int       m_AutoInc;
	int       m_iRunState; // Ready|Run|Pause|Stop
	int       m_iMaxRetryCount;
	string    m_strCharSet;
	
	//Campaign Info
	shared_ptr<stCpMailInfo> m_cpMailInfo;	

	//-Ems DKIM
	unordered_map<std::string, shared_ptr<CEmsDKIM> > *m_pEmsDKIMList;
};

#endif   //__CEMS_THREAD_HEADER__

