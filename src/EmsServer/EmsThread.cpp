#include <string.h>
#include "EmsThread.h"

#include "sth_syslog.h"
#include "EmsDefineString.h"
#include "FuncString.h"
#include "EmsConfig.h"
#include "EmsLog.h"
#include "EmsRMQManager.h" 

#include "EmsDKIM.h"  //--ischoi: Use DKIM

extern CEmsLog * gLog;

char gMessageKey[50] = "";

int giSizeCodeRes_ipfilter_allcheck = 0;
int giSizeCodeRes_ipfilter_453      = 0;
int giSizeCodeRes_unknowuser_550    = 0;
int giSizeCodeRes_unknowuser_505    = 0;
int giSizeCodeRes_temporary_user    = 0;
int giSizeCodeRes_relay_denined     = 0;
int giSizeCodeRes_server_busy       = 0;
int giSizeCodeRes_server_421_busy   = 0;

///-----
const char gStrCodeRes_ipfilter_allcheck[][STRLEN_CodeResult] = {
	"your ip",
	"junk mail",
	"connection refused",
	"spam",
	"block",
	"filter",
	"blacklisted",
	"blocked for spam",
	"black list"
};

const char gStrCodeRes_ipfilter_453[][STRLEN_CodeResult] = {
	"not allowed",
};

const char gStrCodeRes_unknowuser_550[][STRLEN_CodeResult] = {
	"no such user",
	"no such mailbox known",
	"doesn't have",
	"does not exist",
	"disabled or discontinued",
	"mailbox doesn't exist",
	"unknown",
	"invalid recipient address",
	"mailbox disabled",
	"mailbox unavailable",
	"invalid recipient",
	"no thank",
	"no inbox for",
	"invalid address",
	"not valid",
	"mail box",
	"unsupported",
	"accept failed",
	"invalid address",
	"this account",
	"my local list",
	"no mailbox",
	"no mail-box",
	"recipient cannot be verified",
	"user not found",
	"user unknown"
};

const char gStrCodeRes_unknowuser_505[][STRLEN_CodeResult] = {
	"unknown user"
};

const char gStrCodeRes_temporary_user[][STRLEN_CodeResult] = {
	"suspended user",
	"over quota",
	"greenlist.html",
	"out of memory",
	"mailbox full",
	"mailbox is full",
	"insufficient",
	"inactive",
	"many",
	"exceed",
	"temporarily disabled"
};

const char gStrCodeRes_relay_denined[][STRLEN_CodeResult] = {
	"relay",
	"relaying",
	"domain isn't in my list",
	"릴레이",
};

const char gStrCodeRes_server_busy[][STRLEN_CodeResult] = {
	"too busy"
};

const char gStrCodeRes_server_421_busy[][STRLEN_CodeResult] = {
	"temporarily",
	"connect to"
};

void Get_SizeCodeRes()
{
	giSizeCodeRes_ipfilter_allcheck = sizeof(gStrCodeRes_ipfilter_allcheck) / STRLEN_CodeResult;
	giSizeCodeRes_ipfilter_453      = sizeof(gStrCodeRes_ipfilter_453)      / STRLEN_CodeResult;
	giSizeCodeRes_unknowuser_550    = sizeof(gStrCodeRes_unknowuser_550)    / STRLEN_CodeResult;
	giSizeCodeRes_unknowuser_505    = sizeof(gStrCodeRes_unknowuser_505)    / STRLEN_CodeResult;
	giSizeCodeRes_temporary_user    = sizeof(gStrCodeRes_temporary_user)    / STRLEN_CodeResult;
	giSizeCodeRes_relay_denined     = sizeof(gStrCodeRes_relay_denined)     / STRLEN_CodeResult;
	giSizeCodeRes_server_busy       = sizeof(gStrCodeRes_server_busy)       / STRLEN_CodeResult;
	giSizeCodeRes_server_421_busy   = sizeof(gStrCodeRes_server_421_busy)   / STRLEN_CodeResult;
}

int GetExpResultCode(int _SmtpStep, int _iCode3Ch, char* _StrErrExp)
{
	if(_SmtpStep == 21)
	{
		switch(_iCode3Ch)
		{
		case 101:
		case 102:
		case 151:
		case 152:
		case 110:
		case 160:
		case 161:
			return -1;
			break;
		case 201:
			return -3;
			break;
		case 301:
		case 351:
			return -2;
			break;
		default://501,401
			return -125;
			break;
		}
	}

	for(int i=0; i<giSizeCodeRes_server_busy; i++)
	{
		if(strstr(_StrErrExp, gStrCodeRes_server_busy[i]) != NULL)
			return -12;
	}

	for(int i=0; i<giSizeCodeRes_ipfilter_allcheck; i++)
	{
		if(strstr(_StrErrExp, gStrCodeRes_ipfilter_allcheck[i]) != NULL)
			return -123;
	}

	if(_iCode3Ch == 453 || _iCode3Ch == 421)
	{
		for(int i=0; i<giSizeCodeRes_server_421_busy; i++)
		{
			if(strstr(_StrErrExp, gStrCodeRes_server_421_busy[i]) != NULL)
				return -12;
		}
	}

	switch(_iCode3Ch)
	{
	case 550:
	case 551:
	case 554:
	case 553:
	case 511:
	case 501:
	case 452:
		{
			for(int i=0; i<giSizeCodeRes_unknowuser_550; i++)
			{
				if(strstr(_StrErrExp, gStrCodeRes_unknowuser_550[i]) != NULL)
					return -121;
			}
		}
		break;
	case 505:
		{
			for(int i=0; i<giSizeCodeRes_unknowuser_505; i++)
			{
				if(strstr(_StrErrExp, gStrCodeRes_unknowuser_505[i]) != NULL)
					return -121;
			}
		}
		break;
	}

	switch(_iCode3Ch)
	{
	case 550:
	case 551:
	case 554:
	case 552:
	case 522:
	case 452:
	case 450:
		{
			for(int i=0; i<giSizeCodeRes_temporary_user; i++)
			{
				if(strstr(_StrErrExp, gStrCodeRes_temporary_user[i]) != NULL)
					return -122;
			}
		}
		break;
	}

	for(int i=0; i<giSizeCodeRes_relay_denined; i++)
	{
		if(strstr(_StrErrExp, gStrCodeRes_relay_denined[i]) != NULL)
			return -124;
	}

	switch(_SmtpStep)
	{
	case 0:
	case 1:
		return -1;
		break;
	case 2:
		return -4;
		break;		
	case 3:
		return -5;
		break;		
	case 4: 
		return -6;
		break;			
	case 5: 
		return -7;
		break;			
	case 6: 
		return -11;
		break;			
	default:
		return -125;
		break;
	}

	return -125;
}

void Set_MessageKey(char *_pStr)
{
	char *m_p = strchr(_pStr, '.');
	bool m_IsInput = false;

	if(m_p != NULL)
	{
		m_p = strchr(m_p+1, '.');

		if(m_p != NULL)
		{
			srand(time(0));
			int m_RandNuem = rand();
			//char m_TmpStr[50];

			sprintf(gMessageKey, "%d-%s", m_RandNuem, m_p+1);
			m_IsInput = true;

			//strcpy(gMessageKey, m_p+1);
		}
	}

	if(m_IsInput == false)
	{
		srand(time(0));
		int m_RandNuem = rand();
		sprintf(gMessageKey, "%d", m_RandNuem);
	}
}



//######################################### CEmsThread #########################################
CEmsThread::CEmsThread(unsigned long identifier, 
	                     boost::shared_ptr<pthread_mutex_t> pMutex, 
	                     boost::shared_ptr<pthread_cond_t>pCond, 
	                     boost::shared_ptr< queue < boost::shared_ptr < CEmsRequest > > > pQueue)
	: CThread<CEmsRequest>(identifier, pMutex, pCond, pQueue)
	,	m_ThreadMode       (MODE_M)
	, m_pThreadBuf       (NULL)
	, m_ThreadBufLen     (0)
	, m_pEncodeBufTT     (NULL)
	, m_EncodeBufLenTT   (0)
	, m_pTmpBuf          (NULL)
	, m_TmpBufLen        (0)
	, m_pEncodeBufBody   (NULL)
	, m_EncodeBufLenBody (0)
	, m_pWStrBuf         (NULL)
	, m_WStrBufLen       (0)
	, m_SplitCntTT       (0)
	, m_SplitCntBody     (0)
	, m_iRunState        (TSTATE_READY)
	, m_pEmsDKIMList     (NULL)
{
	memset(m_strQuitBuf, 0, SZ_TINYBUFFER);
	snprintf(m_strQuitBuf, SZ_TINYBUFFER, ENHEAD_QUIT CRLF);

	m_AutoInc        = 0;
	m_iMaxRetryCount = theEMSConfig()->getMailRetryCount();
	m_strCharSet     = theEMSConfig()->getServerCharSet();
}

//######################################### ~CEmsThread #########################################
CEmsThread::~CEmsThread()
{
	stopThread();
	if(m_pThreadBuf != NULL){
		delete [] m_pThreadBuf;
		m_pThreadBuf = NULL;
	}

	if(m_pEncodeBufTT != NULL){
		delete [] m_pEncodeBufTT;
		m_pEncodeBufTT = NULL;
	}
	
	if(m_pTmpBuf != NULL){
		delete [] m_pTmpBuf;
		m_pTmpBuf = NULL;
	}

	if(m_pEncodeBufBody != NULL){
		delete [] m_pEncodeBufBody;
		m_pEncodeBufBody = NULL;
	}
	
	if(m_pWStrBuf != NULL){
		delete [] m_pWStrBuf;
		m_pWStrBuf = NULL;
	}
}

//####################################### insertToEWTPQ ########################################
//ReEnqueue Mail Message into ThreadPool Queue
void CEmsThread::enqueueMsg  (shared_ptr<CCpMail> spCpMail)
{
	m_pEmsWorkerThreadPool->addToDomainQueueList(spCpMail);
}

//####################################### First3ChToNum ########################################
int CEmsThread::First3ChToNum(char *pFirst3Ch)
{
	int i=0;

	for(; i<3; i++)
	{
		if(pFirst3Ch[i]>='0' && pFirst3Ch[i]<='9') 
		{
		}
		else
		{
			break;
		}
	}

	if(i==3)
		return atoi(pFirst3Ch);
	else
		return NOT_NUMBER; //-1;
}

//####################################### Check_SmtpCode #######################################
int CEmsThread::Check_SmtpCode(boost::shared_ptr<CEmsRequest> spRequest, char *pFirst3Ch)
{
	//CRequest로부터 Client를 꺼내어 수신한 데이터의 헤더값으로 세자리 꺼내서 리턴함
	bool bIsValidPacket = true;
	bool bIsErrorOccur  = false;
	int  iCode3Ch = NOT_NUMBER;
	
	char tmp_StrErrExp[LEN_ExplainLong +1] ={0,};

	shared_ptr<CCpMail> spCpMail = 	spRequest->getClient()->getCpMail();
	if(spCpMail.get() == NULL) //--ischoi --if error occurred return -1
		return -1;

	if(spRequest->getPacketSize() < 3)
		bIsValidPacket = false;

	if(bIsValidPacket == true)
	{
		strncpy(pFirst3Ch, spRequest->getPacket().get(), 3);
		pFirst3Ch[3] = '\0';

		iCode3Ch = First3ChToNum(pFirst3Ch);
		spCpMail.get()->setSmtpCode(iCode3Ch);

		if(iCode3Ch != NOT_NUMBER)
		{
			if(*(spRequest->getPacket().get() + 3) == '-')
			{
				gLog->Write("[%s][%s][%d] pFirst3CH:[%s]", __FILE__, __FUNCTION__, __LINE__, pFirst3Ch);

				char *pos = strnstr(spRequest->getPacket().get(), pFirst3Ch, spRequest->getPacketSize());

				if(pos == NULL)
				{
					if(spRequest->getPacketSize() < LEN_ExplainLong)
					{
						strncpy(tmp_StrErrExp, spRequest->getPacket().get(), spRequest->getPacketSize());
						tmp_StrErrExp[spRequest->getPacketSize()] = '\0';
						ChangeNewline(tmp_StrErrExp); //9add
					}

					return SMTP_SkipOccur;
				}
				else
				{
					//gLog->Write("Find Mess => %s", mStrErrExp); //strncpy(mStrErrExp 해줘야함
				}
			}
		}
	}
	else
	{
		spCpMail.get()->setSmtpCode(iCode3Ch);
	}

	if(iCode3Ch == NOT_NUMBER)
	{
		bIsErrorOccur = true;

		spCpMail.get()->incRetryCount();

		if(spCpMail.get()->getStepComplete() == true){
			//[Do Nothing!]
			return SMTP_ErrorOccur;
		}
		//else
		//{
		//	gLog->Write("[%s][%s][%d][iQryUdt_CpMail_SmtpStep21Chk7][%s][%s][%s]"
		//	            , __FILE__, __FUNCTION__, __LINE__
		//	            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
		//	            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
		//	            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
    //
		//	gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpStep21Chk7);
		//}

		if(spRequest->getPacketSize() > LEN_Explain)
		{
			//--
			if(spRequest->getPacketSize() > LEN_ExplainLong)
			{
				strncpy(tmp_StrErrExp, spRequest->getPacket().get(), LEN_ExplainLong);
				tmp_StrErrExp[LEN_ExplainLong] = '\0';
				ChangeNewline(tmp_StrErrExp);
			}
			else
			{
				strncpy(tmp_StrErrExp, spRequest->getPacket().get(), spRequest->getPacketSize());
				tmp_StrErrExp[spRequest->getPacketSize()] = '\0';
				ChangeNewline(tmp_StrErrExp);
			}

			strncpy(tmp_StrErrExp, spRequest->getPacket().get(), LEN_Explain);
			tmp_StrErrExp[LEN_Explain] = '\0';
			ChangeNewline(tmp_StrErrExp); 
			//gLog->Write("D[LEN_ExplainLong] V[Midx:%d,%s@%s,TCnt:%d,Explain:%s]"
			//            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			//            , spCpMail.get()->getMailInfo(MHEADER_TOID)
			//            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN)
			//            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN)
			//            , spCpMail.get()->getRetryCount(), tmp_StrErrExp);

		}
		else
		{
			strncpy(tmp_StrErrExp, spRequest->getPacket().get(), spRequest->getPacketSize());
			tmp_StrErrExp[spRequest->getPacketSize()] = '\0';
			ChangeNewline(tmp_StrErrExp); //9add
		}
		
		{
			int iExpResultCode = 0;
			char tmp_StrErrExpLower[LEN_ExplainLong+1]={0,};
			strcpy(tmp_StrErrExpLower, tmp_StrErrExp);
			to_lowercase(tmp_StrErrExpLower);
			iExpResultCode = GetExpResultCode(STEP21, iCode3Ch, tmp_StrErrExpLower);
			spCpMail.get()->setExpResultCode( iExpResultCode );
			spCpMail.get()->setExpResultStr ( tmp_StrErrExp );

			gLog->Write("[iQryIns_CodeExpResult][%s][%s][%s]"
			            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBINSERT, spCpMail, iQryIns_CodeExpResult);

			gLog->Write("[%s][%s][%d][iQryUdt_CpMail_SmtpStep21Chk7][%s][%s][%s]"
			            , __FILE__, __FUNCTION__, __LINE__
			            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );

			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpStep21Chk7);
		}


		return SMTP_ErrorOccur;
	}

	switch(pFirst3Ch[0])
	{
		case '4':
		case '5':
		{
			bIsErrorOccur = true;
		}
		break;
	}

	if(bIsErrorOccur == true)
	{
		switch(spCpMail.get()->getSmtpStep())
		{
			case SMTP_STEP03_MailFromEND:
			case SMTP_STEP04_RcptToEND:
				{
					/*
					//switch(m_iCode3Ch)
					//{
					//case 501:
					//case 511:
					//case 550:
					//case 551:
					//case 552:
					//case 553:
					//case 554:
					//	m_spCpMail->mTryCnt = MAX_TryCnt;
					//	break;
					//}
					*/
					if(iCode3Ch>=500 && iCode3Ch<600)
					{
						spCpMail.get()->setRetryCount(m_iMaxRetryCount);
						break;
					}
				}
				break;
			case SMTP_STEP06_DataContentEND:
				{
					/*
					//switch(m_iCode3Ch)
					//{
					//case 550:
					//case 553:
					//case 554:
					//	m_spCpMail->mTryCnt = MAX_TryCnt;
					//	break;
					//}
					*/
					if(iCode3Ch >= 500 && iCode3Ch < 600)
					{
						spCpMail.get()->setRetryCount(m_iMaxRetryCount);
						break;
					}
				}
				break;
		}
	}

	if(bIsErrorOccur == false)
	{
		if(iCode3Ch == RESP_220){
			//bIsErrorOccur = false;
		}
		else{
			switch(spCpMail.get()->getSmtpStep())
			{
			case SMTP_STEP00_Init:
			case SMTP_STEP01_ConnectEND:
				if(iCode3Ch != 250 && iCode3Ch != 220)
					bIsErrorOccur = true;
				break;
			case SMTP_STEP02_HeloEND:
			case SMTP_STEP03_MailFromEND:
			case SMTP_STEP04_RcptToEND:
				if(iCode3Ch != RESP_250)
					bIsErrorOccur = true;
				break;

			case SMTP_STEP05_DataEND:
				if(iCode3Ch != RESP_354)
					bIsErrorOccur = true;
				break;
			case SMTP_STEP06_DataContentEND:
				if(iCode3Ch != RESP_250)
					bIsErrorOccur = true;
				else{
					spCpMail.get()->setStepComplete(true);
					gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpStepComplete);
				}
				break;
			case SMTP_STEP07_QuitOrRsetEnd:
				if(iCode3Ch != RESP_250 && iCode3Ch != RESP_221)
					bIsErrorOccur = true;
				break;
			}
		}
	}

	if(bIsErrorOccur == true)
	{
		if(spCpMail.get()->getRetryCount() < m_iMaxRetryCount)
			spCpMail.get()->incRetryCount();
			
		if(spCpMail.get()->getStepComplete() == true)
		{ //[Do Nothing!]
			return SMTP_ErrorOccur;
		}
		//else
		//{
		//	gLog->Write("[iQryUdt_CpMail_SmtpChk7][%s][%s][%s]"
		//	            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
		//	            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
		//	            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
		//	gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpChk7);
		//}

		if(spRequest->getPacketSize() > LEN_Explain)
		{
			strncpy(tmp_StrErrExp, spRequest->getPacket().get(), LEN_Explain);
			tmp_StrErrExp[LEN_Explain] = '\0';
			ChangeNewline(tmp_StrErrExp); //9add
		}
		else
		{
			strncpy(tmp_StrErrExp, spRequest->getPacket().get(), spRequest->getPacketSize());
			tmp_StrErrExp[spRequest->getPacketSize()] = '\0';
			ChangeNewline(tmp_StrErrExp); //9add
		}

		bool bIsInserResultCode = true;
		int iExpResultCode = 0;  //<== spCpMail에 저장함

		if(bIsInserResultCode == true)
		{
			char tmp_StrErrExpLower[LEN_ExplainLong+1]={0,};
			strcpy(tmp_StrErrExpLower, tmp_StrErrExp);
			to_lowercase(tmp_StrErrExpLower);
			iExpResultCode = GetExpResultCode(spCpMail.get()->getSmtpStep(), iCode3Ch, tmp_StrErrExpLower);
			spCpMail.get()->setExpResultCode( iExpResultCode );
			spCpMail.get()->setExpResultStr ( tmp_StrErrExp );
		}

		if(bIsInserResultCode == true)
		{
			gLog->Write("[iQryIns_CodeExpResult][%s][%s][%s]"
			            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBINSERT, spCpMail, iQryIns_CodeExpResult);
		}
		else
		{
			gLog->Write("[iQryIns_CodeExp][%s][%s][%s]"
			            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBINSERT, spCpMail, iQryIns_CodeExp);

		}
		
		gLog->Write("[iQryUdt_CpMail_SmtpChk7][%s][%s][%s]"
		            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
		gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpChk7);

		return SMTP_ErrorOccur;
	}

	return iCode3Ch;
}
//####################################### Check_SmtpCode ######################################


//-----------------------------------------------
// MODE_M : Mail Campaign Information(Get/Set) 
//-----------------------------------------------
void CEmsThread::setCpMailInfo(shared_ptr<stCpMailInfo> spCpInfo)
{
	int tmpTitleLen = spCpInfo.get()->_icpMailTitleLen;
	int tmpBodyLen  = spCpInfo.get()->_icpMailBodyLen ;

	float mVal = 1.5; //사용할 메모리에 대해서 메시지 크기를 반영하여 동적으로 크기를 할당:+30%

	int tmpEncodeBufLenTT   = (int)(tmpTitleLen*mVal) + SIZE_HeaderBuf;
	int tmpEncodeBufLenBody = (int)(tmpBodyLen*mVal) + SIZE_EncodeBuf + SIZE_HeaderBuf;
	int tmpThreadBufLen     = (int)(tmpEncodeBufLenTT + tmpEncodeBufLenBody) + SIZE_ThreadBuf;
	
	//------(m_pThreadBuf)-----
	if(m_pThreadBuf != NULL){
		delete [] m_pThreadBuf;
		m_pThreadBuf   = NULL;
		m_ThreadBufLen = 0;
	}

	if( tmpThreadBufLen > m_ThreadBufLen ){
		m_ThreadBufLen = tmpThreadBufLen;
		m_pThreadBuf   = new char[m_ThreadBufLen];
		memset(m_pThreadBuf, 0, m_ThreadBufLen);
	}

	//------(m_pEncodeBufTT)-----
	if(m_pEncodeBufTT != NULL){
		delete [] m_pEncodeBufTT;
		m_pEncodeBufTT   = NULL;
		m_EncodeBufLenTT = 0;
	}

	if( tmpEncodeBufLenTT > m_EncodeBufLenTT ){
		m_EncodeBufLenTT = tmpEncodeBufLenTT;
		m_pEncodeBufTT   = new char[m_EncodeBufLenTT];
		memset(m_pEncodeBufTT, 0, m_EncodeBufLenTT);
	}
	
	//------(m_pTmpBuf)-----
	if(m_pTmpBuf != NULL){
		delete [] m_pTmpBuf;
		m_pTmpBuf   = NULL;
		m_TmpBufLen = 0;
	}

	if( m_EncodeBufLenTT > m_TmpBufLen ){
		m_TmpBufLen = m_EncodeBufLenTT;
		m_pTmpBuf   = new char[m_TmpBufLen];
		memset(m_pTmpBuf, 0, m_TmpBufLen);
	}

	//------(m_pEncodeBufBody)-----
	if(m_pEncodeBufBody != NULL){
		delete [] m_pEncodeBufBody;
		m_pEncodeBufBody   = NULL;
		m_EncodeBufLenBody = 0;
	}

	if( tmpEncodeBufLenBody > m_EncodeBufLenBody ){
		m_EncodeBufLenBody = tmpEncodeBufLenBody;
		m_pEncodeBufBody   = new char[m_EncodeBufLenBody];
		memset(m_pEncodeBufBody, 0, m_EncodeBufLenBody);
	}

	//------(m_pWStrBuf)-----
	if(m_pWStrBuf != NULL){
		delete [] m_pWStrBuf;
		m_pWStrBuf   = NULL;
		m_WStrBufLen = m_EncodeBufLenBody;
		m_pWStrBuf   = new char[m_WStrBufLen];
		memset(m_pWStrBuf, 0, m_WStrBufLen);
	}
	
	m_cpMailInfo = spCpInfo;
	
	shared_ptr<CCpMail> spCpMail;
		 
	preDataProcess(spCpMail, m_stSplitInfoTT, m_SplitCntTT, m_stSplitInfoBody, m_SplitCntBody);
}

//-----------------------------------------------
// MODE_M : Mail Campaign Information(Get/Set) 
//-----------------------------------------------
void CEmsThread::InitThreadBuf()
{
	int tmpEncodeBufLenTT   = SIZE_EncodeBuf;
	int tmpEncodeBufLenBody = SIZE_EncodeBuf;
	int tmpThreadBufLen     = SIZE_ThreadBuf;

	//------(m_pEncodeBufTT)-----
	if(m_pEncodeBufTT != NULL){
		delete [] m_pEncodeBufTT;
		m_pEncodeBufTT = NULL;
	}

	if( tmpEncodeBufLenTT > m_EncodeBufLenTT ){
		m_EncodeBufLenTT = tmpEncodeBufLenTT;
		m_pEncodeBufTT   = new char[m_EncodeBufLenTT];
		memset(m_pEncodeBufTT, 0, m_EncodeBufLenTT);
	}
	
	//------(m_pTmpBuf)-----
	if(m_pTmpBuf != NULL){
		delete [] m_pTmpBuf;
		m_pTmpBuf = NULL;
	}

	if( m_EncodeBufLenTT > m_TmpBufLen ){
		m_TmpBufLen = m_EncodeBufLenTT;
		m_pTmpBuf   = new char[m_TmpBufLen];
		memset(m_pTmpBuf, 0, m_TmpBufLen);
	}

	//------(m_pEncodeBufBody)-----
	if(m_pEncodeBufBody != NULL){
		delete [] m_pEncodeBufBody;
	}

	if( tmpEncodeBufLenBody > m_EncodeBufLenBody ){
		m_EncodeBufLenBody = tmpEncodeBufLenBody;
		m_pEncodeBufBody   = new char[m_EncodeBufLenBody];
		memset(m_pEncodeBufBody, 0, m_EncodeBufLenBody);
	}

	//------(m_pWStrBuf)-----
	if(m_pWStrBuf != NULL){
		delete [] m_pWStrBuf;
	}

	if( m_EncodeBufLenTT > m_EncodeBufLenBody ){
		m_WStrBufLen = m_EncodeBufLenTT;
	}
	else{
		m_WStrBufLen = m_EncodeBufLenBody;
	}

	if(m_WStrBufLen > 0){
		m_pWStrBuf  = new char[m_WStrBufLen];
		memset(m_pWStrBuf, 0, m_WStrBufLen);
	}

	//------(m_pThreadBuf)-----
	if(m_pThreadBuf != NULL){
		delete [] m_pThreadBuf;
		m_pThreadBuf = NULL;
	}

	if( tmpThreadBufLen > m_ThreadBufLen ){
		m_ThreadBufLen = tmpThreadBufLen;
		m_pThreadBuf   = new char[m_ThreadBufLen];
		memset(m_pThreadBuf, 0, m_ThreadBufLen);
	}
}


//-----------------------------------------------
// MODE_M : Get Mail Information
//-----------------------------------------------
shared_ptr<stCpMailInfo> CEmsThread::getCpMailInfo()
{
	return m_cpMailInfo;
}


//-----------------------------------------------
// Thread Buffer Reset Function 
//-----------------------------------------------
bool CEmsThread::resetBuffer(shared_ptr<CCpMail> spCpMail, int iMode)
{
	if(iMode==MODE_M){
		memset(m_pEncodeBufTT,   0, m_EncodeBufLenTT);
		memset(m_pEncodeBufBody, 0, m_EncodeBufLenBody);
		memset(m_pTmpBuf,        0, m_TmpBufLen);
		memset(m_pWStrBuf,       0, m_WStrBufLen);
		memset(m_pThreadBuf,     0, m_ThreadBufLen);
	}
	else{ //if(iMode==MODE_I)
		
		int tmpTitleLen = strlen(spCpMail.get()->getMailInfo(MHEADER_MAILTITLE));
		int tmpBodyLen  = strlen(spCpMail.get()->getMailInfo(MAIL_MSG_BODY));
	
		float mVal = 1.5; //사용할 메모리에 대해서 메시지 크기를 반영하여 동적으로 크기를 할당:+30%
	
		int tmpEncodeBufLenTT   = (int)(tmpTitleLen*mVal) + SIZE_HeaderBuf;
		int tmpEncodeBufLenBody = (int)(tmpBodyLen*mVal) + SIZE_EncodeBuf + SIZE_HeaderBuf;
		int tmpThreadBufLen     = (int)(tmpEncodeBufLenTT + tmpEncodeBufLenBody) + SIZE_ThreadBuf;
		
		//------(m_pEncodeBufTT)-----
		if( tmpEncodeBufLenTT > m_EncodeBufLenTT ){
			if(m_pEncodeBufTT != NULL){
				delete [] m_pEncodeBufTT;
				m_pEncodeBufTT = NULL;
			}
			m_EncodeBufLenTT = tmpEncodeBufLenTT;
			m_pEncodeBufTT   = new char[m_EncodeBufLenTT];
		}
		else{
			if(m_pEncodeBufTT == NULL){
				m_EncodeBufLenTT = tmpEncodeBufLenTT;
				m_pEncodeBufTT  = new char[m_EncodeBufLenTT];
			}
		}
		memset(m_pEncodeBufTT, 0, m_EncodeBufLenTT);
		
		//------(m_pEncodeBufBody)-----
		if( tmpEncodeBufLenBody > m_EncodeBufLenBody ){
			if(m_pEncodeBufBody != NULL){
				delete [] m_pEncodeBufBody;
				m_pEncodeBufBody = NULL;
			}
			m_EncodeBufLenBody = tmpEncodeBufLenBody;
			m_pEncodeBufBody   = new char[m_EncodeBufLenBody];
		}
		else{
			if(m_pEncodeBufBody == NULL){
				m_EncodeBufLenBody = tmpEncodeBufLenBody;
				m_pEncodeBufBody   = new char[m_EncodeBufLenBody];
			}
		}
		memset(m_pEncodeBufBody, 0, m_EncodeBufLenBody);
	
		//------(m_pTmpBuf)-----
		if( m_EncodeBufLenTT > m_TmpBufLen ){
			if(m_pTmpBuf != NULL){
				delete [] m_pTmpBuf;
				m_pTmpBuf = NULL;
			}
			m_TmpBufLen = m_EncodeBufLenTT;
			m_pTmpBuf   = new char[m_TmpBufLen];
		}
		else{
			if(m_pTmpBuf == NULL){
				m_TmpBufLen = m_EncodeBufLenTT;
				m_pTmpBuf   = new char[m_TmpBufLen];
			}
		}
		memset(m_pTmpBuf, 0, m_TmpBufLen);

		//------(m_pWStrBuf)-----
		if( tmpThreadBufLen > m_WStrBufLen ){
			if(m_pWStrBuf != NULL){
				delete [] m_pWStrBuf;
				m_pWStrBuf   = NULL;
			}
			m_WStrBufLen = tmpThreadBufLen;
			m_pWStrBuf  = new char[m_WStrBufLen];
		}
		else{
			if(m_pWStrBuf == NULL){
				m_WStrBufLen = tmpThreadBufLen;
				m_pWStrBuf  = new char[m_WStrBufLen];
			}
		}
		memset(m_pWStrBuf, 0, m_WStrBufLen);

		//------(m_pThreadBuf)-----
		if( tmpThreadBufLen > m_ThreadBufLen ){
			if(m_pThreadBuf != NULL){
				delete [] m_pThreadBuf;
				m_pThreadBuf   = NULL;
			}
			m_ThreadBufLen = tmpThreadBufLen;
			m_pThreadBuf   = new char[m_ThreadBufLen];
		}
		else{
			if(m_pThreadBuf == NULL){
				m_ThreadBufLen = tmpThreadBufLen;
				m_pThreadBuf   = new char[m_ThreadBufLen];
			}
		}
		memset(m_pThreadBuf, 0, m_ThreadBufLen);
	}

	//gLog->Write("[%s][SIZE]:[m_ThreadBufLen : %d]", __FUNCTION__, m_ThreadBufLen);
	return true;
}

//-----------------------------------------------
// Add To Header Function 
//-----------------------------------------------
int CEmsThread::addToBufferLine(char *pHeaderBuf, char * pMsgStr)
{
	if((pHeaderBuf != NULL)&&(pMsgStr != NULL)){
		int hb_len = strlen(pHeaderBuf);
		strcpy(&pHeaderBuf[hb_len], pMsgStr);
		return strlen(pHeaderBuf) - hb_len;
	}
	else{
		if(pHeaderBuf == NULL)
			return -1;
		else//if (pMsgStr == NULL)
			return -2;
	}	
}

//-----------------------------------------------
// Add To Header Function 
//-----------------------------------------------
void CEmsThread::setPause()
{
	setRunState(TSTATE_PAUSE);
}

//-----------------------------------------------
// Set EmsThread Restart
//-----------------------------------------------
void CEmsThread::setRestart()
{
	setRunState(TSTATE_RUN);
}


//-----------------------------------------------
//preprocess() 리턴 값이 true 인 경우, 전달된 pRequest를 processRequest()에서 처리하고,
//리턴 값이 false 인 경우, processRequest() 실행되지 않음.
//-----------------------------------------------
bool CEmsThread::preprocess()
{
	if((getThreadMode() == MODE_M) && m_cpMailInfo.get() == NULL){
		//int iThreadMode = getThreadMode();
		//gLog->Write("[%s][%s][getThreadMode()=%d][CpMailInfo is %s]", __FILE__, __FUNCTION__, (int)iThreadMode, (m_cpMailInfo.get()==NULL?"NULL":"Not NULL"));
		usleep(THREAD_SLEEPTIME_1SEC);
		return false;
	}

	if(getRunState()==TSTATE_STOP){
		//Process Stop
		//gLog->Write("[%s][%s][getRunState()==TSTATE_STOP][%d]", __FILE__, __FUNCTION__, (int)getIdentifier());
		return true;
	}
	else if(getRunState()==TSTATE_PAUSE){
		//Process Pause State
		//gLog->Write("[%s][%s][getRunState()==TSTATE_PAUSE][%d]", __FILE__, __FUNCTION__, (int)getIdentifier());
		usleep(THREAD_SLEEPTIME_10MSEC);
		return false;
	}
	else if(getRunState()==TSTATE_RUN){
		//Proess RUN state
		//gLog->Write("[%s][%s][getRunState()==TSTATE_RUN][%d]", __FILE__, __FUNCTION__, (int)getIdentifier());
		return true;
	}

	return true;
}

//-----------------------------------------------
// MODE_M : 함수 이전에 수신된 Ems Campaign 정보를 처리/저장
//-----------------------------------------------
void CEmsThread::preDataProcess(shared_ptr<CCpMail> spCpMail, shared_ptr<stSplitInfo> &spSplitInfoTT, int &iSplitCntTT, shared_ptr<stSplitInfo> &spSplitInfoBody, int &iSplitCntBody)
{
	
	//=============================================================================
	//Wide String 을 사용하는 경우
	//pMailTitle / pMailBody를 이용하여 메일 데이터 생성
	// 우선 Title 포인터를 가져온다.
	// Title을 파싱하면서 Title 데이터를 재구성한다.
	//m_pThreadBuf 버퍼에는 전체메시지가 들어가야함.

	//wide string  처리: m_pWStrBuf
	//여기서 처리하는 부분은 함수로 분리하여 처리할 예정.
	//실시간으로 검색 복사 치환의 순서대로 처리
	
	//Mail Info
	char * tmpMailMsg = NULL;
	char * pMsgPos    = NULL;
	char * pPos       = NULL;
	char * pSave      = NULL;
	char   tmpWCh     = '\0';
	int    iPos       = 0;

	stSplitInfo  *pSplitInfo = NULL;
	
	
	//-----------------------------------------
	//#define SPLIT_ByBody		0 //mUseWstr
	//#define SPLIT_ByTitle		1 //mUseWstrTT
	//#define SPLIT_END       2 //
	//-----------------------------------------
	for(int spSwitch=0; spSwitch<SPLIT_END; spSwitch++){
		
		if(spSwitch == SPLIT_ByTitle){
			if(getThreadMode() == MODE_M){
				tmpMailMsg = getCpMailInfo().get()->_cpMailTitle;
				pMsgPos    = getCpMailInfo().get()->_cpMailTitle;
			}
			else {  //MODE_I
				tmpMailMsg = const_cast<char *>(spCpMail.get()->getMailInfo(MHEADER_MAILTITLE));
				pMsgPos    = const_cast<char *>(spCpMail.get()->getMailInfo(MHEADER_MAILTITLE));
			}
			pPos       = NULL;
			pSave      = NULL;
			tmpWCh     = '\0';
			iPos       = 0;
			pSplitInfo = NULL;
		}
		else{
			if(getThreadMode() == MODE_M){
				tmpMailMsg = getCpMailInfo().get()->_cpMailBody;
				pMsgPos    = getCpMailInfo().get()->_cpMailBody;
			}
			else {  //MODE_I
				tmpMailMsg = const_cast<char *>(spCpMail.get()->getMailInfo(MAIL_MSG_BODY));
				pMsgPos    = const_cast<char *>(spCpMail.get()->getMailInfo(MAIL_MSG_BODY));
			}
			pPos       = NULL;
			pSave      = NULL;
			tmpWCh     = '\0';
			iPos       = 0;
			pSplitInfo = NULL;
		}
		
		//-------------------------------------------------------------
		//WideString의 개수를 파악한다
		//-------------------------------------------------------------

		int spInfoCount=0;
		while(true){
			if(pMsgPos == NULL)
				break;
				
			pPos = strstr(pMsgPos, "{item");
	
			if(pPos == NULL)
				break;
			
			tmpWCh = *(pPos + 5);
			
			if(tmpWCh == '\0')
				break;
	
			if( ( ( tmpWCh >= '0' && tmpWCh < '9') || tmpWCh=='N' || tmpWCh=='E'  || tmpWCh=='C'  || tmpWCh=='M'  || tmpWCh=='I' || tmpWCh=='D' )  && *(pPos+6)== '}')
			{
				spInfoCount++;
				pMsgPos = pPos + 7;
			}
			else
			{
				pMsgPos = pPos + 5;
			}
		}//--End While(count wide string)
		

		if(spInfoCount > 0){ // WideString이 있다면
			if(spSwitch == SPLIT_ByTitle){
				spSplitInfoTT = shared_ptr<stSplitInfo>(new stSplitInfo[spInfoCount+1], array_deleter<stSplitInfo>());
				pSplitInfo    = spSplitInfoTT.get();
			}
			else{
				spSplitInfoBody = shared_ptr<stSplitInfo>(new stSplitInfo[spInfoCount+1], array_deleter<stSplitInfo>());
				pSplitInfo      = spSplitInfoBody.get();
			}
			
			pMsgPos = tmpMailMsg;
			pSave   = tmpMailMsg;
			pPos    = NULL;
			tmpWCh  = '\0';
			iPos    = 0;
	
			//m_stSplitInfoTT: Mail Title Split Information 저장
			for(;iPos < spInfoCount; iPos++){
	
				pPos = strstr(pMsgPos, "{item");
	
				if(pPos == NULL)
					break;
				
				tmpWCh = *(pPos+5);
				
				if(tmpWCh == '\0')
					break;
	
				if( ( ( tmpWCh >= '0' && tmpWCh < '9') || tmpWCh=='N' || tmpWCh=='E'  || tmpWCh=='C'  || tmpWCh=='M'  || tmpWCh=='I' || tmpWCh=='D' )  && *(pPos+6)== '}')
				{
					if(pPos == tmpMailMsg) //처음부터 있는경우
					{
						if(pSave)
						{
							pSplitInfo[iPos]._pchSplitPos = pSave; //문자열이 있는 위치
							pSplitInfo[iPos]._iSplitLen   = 0;     //pPos - pSave; 이전 문자열위치에서 현재 치환 문자열까지 거리(치환문자열사이의 문자열 크기)
							pSave = NULL;
						}
					}
					else
					{
						//검색 시작 기준위치와 찾은 위치가 같거나 다르거나 똑같이 증가 시키고 타입 값을 넣는다.
						if(pSave)
						{
							pSplitInfo[iPos]._pchSplitPos = pSave;        //문자열이 있는 위치
							pSplitInfo[iPos]._iSplitLen   = pPos - pSave; //이전 문자열위치에서 현재 치환 문자열까지 거리(치환문자열사이의 문자열 크기)
							pSave = NULL;
						}
						else
						{
							pSplitInfo[iPos]._pchSplitPos = pMsgPos;
							pSplitInfo[iPos]._iSplitLen   = pPos - pMsgPos;
						}
					}
	
					switch(tmpWCh)
					{
					case 'N': //CpMail->ToName
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_Name;
						break;
					case 'E': //Campain->SenderEmail
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_Email;
						break;
					case 'C': //<=CpIdx
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_CpIdx;
						break;
					case 'M': //<=CpMail->MailIdx
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_CpMail_No;
						break;
					case 'I': //<=CpMail->ToId
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_ToId;
						break;
					case 'D': //<=CpMail->ToDomain
						pSplitInfo[iPos]._iSplitInfoType = SplitReserve_ToDomain;
						break;
					default:
						//-- 설정되어있지 않은 타입은 어떻게 처리할지 고민해보자!'Z'인 경우 -6 = SplitReserve_ToDomain이 되어버림
						pSplitInfo[iPos]._iSplitInfoType = tmpWCh-'0'; 
						break;
					}
	
					pMsgPos = pPos + 7;   //검색 시작 포인트 설정
				}
				else //{item이 아닌 경우
				{
					if(pSave == NULL)     //{itemq{itemq인경우 해결하기 위해
						pSave = pMsgPos;    //pSave 저장 포인트
	
					pMsgPos = pPos + 5;   // 검색 시작 포인트 설정
				}
				
			}  //--End for() Loop  widestring 위치 검색 종료
			
			if(spInfoCount > 0) 
			//---------------------------------------------------------------------
			{
				if(pSave != NULL)
				{
					pSplitInfo[spInfoCount]._pchSplitPos = pSave; 
					pSplitInfo[spInfoCount]._iSplitLen   = tmpMailMsg + strlen(tmpMailMsg) - pSave;
				}
				else
				{
					pSplitInfo[spInfoCount]._pchSplitPos = pMsgPos; 
					pSplitInfo[spInfoCount]._iSplitLen   = tmpMailMsg + strlen(tmpMailMsg) - pMsgPos;
				}
				
				pSplitInfo[spInfoCount]._iSplitInfoType = SplitReserve_NULL;
				
				if(spSwitch == SPLIT_ByTitle){
					iSplitCntTT = spInfoCount;
				}
				else{
					iSplitCntBody = spInfoCount;
				}
			}
		}		
	}
}


//-----------------------------------------------
// Mail 메시지 생서 처리를 하기위한 Wide String을 처리
//-----------------------------------------------
shared_ptr<stSplitInfo>  CEmsThread::getWideString(shared_ptr<CCpMail> spCpMail, int &ret)
{ 
	shared_ptr<stSplitInfo>  spWStrSplitInfo;
	stSplitInfo             *pwsSplitInfo = NULL;

	ret = 0;  //Received return value set 0.

	//--------------------------------------------------------------
	bool  bUseWstr  = (atoi(spCpMail.get()->getMailInfo(MHEADER_USEWSTR))>0) ? true:false;
	char *pWideStr  = const_cast<char*>(spCpMail.get()->getMailInfo(MHEADER_WIDESTR));
	int   wsLen     = 0;
	int   iwstrCnt  = 0;
	int   iMaxWSCnt = 0;
	//--------------------------------------------------------------
	//gLog->Write("[%s][%s][%d][USEWSTR:%s][WSTRING:%s]", __FILE__, __FUNCTION__, __LINE__,
	//              (bUseWstr==true?"USE":"NOT USE"), (pWideStr!=NULL?pWideStr:"--"));
	if(pWideStr != NULL){
		wsLen = strlen(pWideStr);
	}
	else{
		return spWStrSplitInfo;
	}
	
	if((bUseWstr == true) && (pWideStr != NULL))
	{
		char *pWStrPos = pWideStr;
		char *pPos     = NULL;

		iwstrCnt = 0;
		
		while(true)
		{
			pPos = strchr(pWStrPos, WIDESTR_ITER);

			if(pPos == NULL){ //No More WIDESTR_ITER!
				if(pWStrPos != (pWideStr + wsLen))
					iwstrCnt++;
				break;
			}
			else
				iwstrCnt++;
				pWStrPos = pPos+1;
		}
		
		spWStrSplitInfo = shared_ptr<stSplitInfo>(new stSplitInfo[iwstrCnt], array_deleter<stSplitInfo>());
		pwsSplitInfo    = spWStrSplitInfo.get();
		
		iMaxWSCnt = iwstrCnt;
		iwstrCnt  = 0;
		pWStrPos  = const_cast<char *>(pWideStr);
		pPos      = NULL;
		
		while(true)
		{
			pPos = strchr(pWStrPos, WIDESTR_ITER);                                 
			
			if(pPos == NULL) //더이상 없으므로 Save & Out!
			{
				if(pWStrPos != pWideStr + wsLen)
				{
					pwsSplitInfo[iwstrCnt]._pchSplitPos    = pWStrPos;
					pwsSplitInfo[iwstrCnt]._iSplitLen      = const_cast<char *>(pWideStr) + wsLen - pWStrPos;
					pwsSplitInfo[iwstrCnt]._iSplitInfoType = iwstrCnt;
					iwstrCnt++;
				}
				break;
			}
			
			if(pWStrPos != pPos)
			{
				pwsSplitInfo[iwstrCnt]._pchSplitPos    = pWStrPos;
				pwsSplitInfo[iwstrCnt]._iSplitLen      = pPos - pWStrPos;
				pwsSplitInfo[iwstrCnt]._iSplitInfoType = iwstrCnt;
				iwstrCnt++;
				
				if(iwstrCnt >= iMaxWSCnt)
					break;
			}
			pWStrPos = pPos + 1;
		}
		ret = iwstrCnt;
	}
	else
		ret = -1;  //Not use wide string
	
	return spWStrSplitInfo;
}


//-------------------------------------------------------------
// [MODE_I]
// WideString Process 함수 처리부분
// WideString 치환 처리
//(처리된 Campaign 정보가 있으므로 정보를 바탕으로 메시지 처리를 함)
// pWSBuf   : Wide String 결과 데이터가 저장
// spCpMail : 메일 입력 정보를 포함하고 있는 구조체
// spSwitch : 메일 메시지 구분(Title(1) / Body(0))
//-------------------------------------------------------------
void CEmsThread::doWideStrProc(char * pWSBuf, shared_ptr<CCpMail> spCpMail, shared_ptr<stSplitInfo> spMsgSplitInfo, int iSplitInfoCount, shared_ptr<stSplitInfo> spWStrSplitInfo, int iWSCount, int spSwitch)
{ 
	//Mail Info
	//--------------------------------------------------------------
	stSplitInfo *pSplitInfo   = spMsgSplitInfo.get();
	stSplitInfo *pWSSplitInfo = spWStrSplitInfo.get();

	int   spInfoCount = iSplitInfoCount;
	int   wsLenSum    = 0;
	int   iType       = 0;
	
	//-----------------------------------------------------
	//UseWstr이 true인 경우에는 아래의 동작을 한다.
	//-----------------------------------------------------
	if(spInfoCount > 0)
	{
		const char *pHeader   = NULL;
		int         headerLen = 0;

		for(int i=0; i < spInfoCount; i++)
		{
			pHeader   = NULL;
			headerLen = 0;
			iType     = pSplitInfo[i]._iSplitInfoType;

			//--메일 메시지를 저장한다.
			if(pSplitInfo[i]._iSplitLen > 0){
				memcpy(pWSBuf+wsLenSum, pSplitInfo[i]._pchSplitPos, pSplitInfo[i]._iSplitLen);
				wsLenSum += pSplitInfo[i]._iSplitLen;
			}

			if(iType < SplitContent_StartNum)
			{
				//TMP LOG
				if(iType < SplitReserve_NULL)
				{
					switch(iType)
					{
					case SplitReserve_Name: //CpMail->to_name
						{
							pHeader   = spCpMail.get()->getMailInfo(MHEADER_TONAME);
							headerLen = strlen(pHeader);
							break;
						}
					case SplitReserve_Email: //Campain->sender_email
						{
							if(spCpMail.get()->getMode()==MODE_M){
								if(m_cpMailInfo.get()->_cpSenderMail != NULL){
									pHeader = m_cpMailInfo.get()->_cpSenderMail;
								}
								else{
									pHeader = NULLSTRING;
								}
							}
							else{
								pHeader = spCpMail.get()->getMailInfo(MHEADER_SENDEREMAIL);
							}
							headerLen = strlen(pHeader);
							break;
						}
					case SplitReserve_CpIdx: //CpIdx -> mailkey
						{
							if(spCpMail.get()->getMode()==MODE_M){
								if(m_cpMailInfo.get()->_cpIndex != NULL){
									pHeader = m_cpMailInfo.get()->_cpIndex;
								}
								else{
									pHeader = NULLSTRING;
								}
							}
							else{
								pHeader   = spCpMail.get()->getMailInfo(MHEADER_CPMAILKEY);
							}
							headerLen = strlen(pHeader);
							break;
						}
					case SplitReserve_CpMail_No: //CpMail->mailidx
						{
							pHeader   = spCpMail.get()->getMailInfo(MHEADER_MAILINDEX);
							headerLen = strlen(pHeader);
							break;
						}
					case SplitReserve_ToId: //CpMail->ToId
						{
							pHeader   = spCpMail.get()->getMailInfo(MHEADER_TOID);
							headerLen = strlen(pHeader);
							break;
						}
					case SplitReserve_ToDomain:  //CpMail->ToDomain
						{
							pHeader   = spCpMail.get()->getMailInfo(MHEADER_TODOMAIN);
							headerLen = strlen(pHeader);
							break;
						}
					}

					//--WideString을 저장한다.
					if( (pHeader != NULL) && (headerLen > 0) ){
						memcpy(pWSBuf+wsLenSum, pHeader, headerLen);
						wsLenSum += headerLen;
					}
					//gLog->Write("[%s][%s][%d][Type:%d][%d][Header:%s][Msg:%s]"
					//            , __FILE__, __FUNCTION__, __LINE__, i, iType, pHeader, pWSBuf);
				}
				else //(iType >= SplitReserve_NULL)  //WideString List에서 찾아 넣어야 함
				{
					//--WideString을 저장한다.
					if(iType < iWSCount){
						memcpy(pWSBuf+wsLenSum, pWSSplitInfo[iType]._pchSplitPos, pWSSplitInfo[iType]._iSplitLen);
						wsLenSum += pWSSplitInfo[iType]._iSplitLen;
					}
				}
			}
			else // if(iType < SplitContent_StartNum) 
			{
				//[Do Nothing!]
			}
		}
		
		if( (pSplitInfo[spInfoCount]._iSplitInfoType == SplitReserve_NULL)
		     && (pSplitInfo[spInfoCount]._iSplitLen > 0) )
		{
			//--남아있는 메일 메시지를 저장한다.
			memcpy(pWSBuf+wsLenSum, pSplitInfo[spInfoCount]._pchSplitPos, pSplitInfo[spInfoCount]._iSplitLen);
			wsLenSum += pSplitInfo[spInfoCount]._iSplitLen;
			pWSBuf[wsLenSum] = '\0';
		}
	}
	else{
		//--남아있는 메일 메시지를 저장한다.
		const char *tmpStr = NULL;

		if( spSwitch == SPLIT_ByTitle){
			if(getThreadMode() == MODE_M){
				tmpStr = getCpMailInfo().get()->_cpMailTitle;
			}
			else {  //MODE_I
				tmpStr = spCpMail.get()->getMailInfo(MHEADER_MAILTITLE);
			}
		}
		else{
			if(getThreadMode() == MODE_M){
				tmpStr = getCpMailInfo().get()->_cpMailBody;
			}
			else {  //MODE_I
				tmpStr = spCpMail.get()->getMailInfo(MAIL_MSG_BODY);
			}
		}

		memcpy(pWSBuf, tmpStr, strlen(tmpStr));
		wsLenSum = strlen(tmpStr);
		pWSBuf[wsLenSum] = '\0';
	}
}


//-------------------------------------------------------------
// m_pEmsDKIMLIst로부터 domainName에 해당하는 
// EmsDKIM 값을 리턴.
// domainName : EmsDKIM을 가져올 키값(도메인 이름으로 매핑)
//-------------------------------------------------------------
shared_ptr<CEmsDKIM>  CEmsThread::getEmsDKIM(const char *domainName)
{
	shared_ptr<CEmsDKIM> tmpEmsDKIM;
	if(m_pEmsDKIMList == NULL)
		return tmpEmsDKIM;

	unordered_map<std::string, shared_ptr<CEmsDKIM> >::iterator itr_dkim = m_pEmsDKIMList->find(domainName);
	if(itr_dkim != m_pEmsDKIMList->end())
		return itr_dkim->second;
	else
		return tmpEmsDKIM;
}

//-------------------------------------------------------------
// Mail Index와 시간 값으로 ID를 생성하여 문자열로 반환
// pIdxTimeBuffer : 생성된 데이터를 반환할 버퍼
// msgKey         : 
// incNum
//-------------------------------------------------------------
void CEmsThread::setIndexDate(char *pIdxTimeBuffer,  struct tm *ptTime, char *msgKey, int incNum)
{
	if(ptTime != NULL){
		
		sprintf(pIdxTimeBuffer, "%ld%s-%d%02d%02d%02d%02d%02d%d"
		                      , getIdentifier()
		                      , msgKey
		                      , ptTime->tm_year + 1900
		                      , ptTime->tm_mon  + 1
		                      , ptTime->tm_mday + 1
		                      , ptTime->tm_hour
		                      , ptTime->tm_min
		                      , ptTime->tm_sec
		                      , incNum);
		
		//gLog->Write("[%s][%s][%d] [INDEX-DATEINFO](IndexTimeBuffer)[%s]", __FILE__, __FUNCTION__, __LINE__, pIdxTimeBuffer);
	}
	else
		gLog->Write("[%s][%s][%d] [struct tm *ptTime is NULL]", __FILE__, __FUNCTION__, __LINE__);
}


//-----------------------------------------------
// 전송할 메시지를 버퍼에 저장
// pSendMsgBuf : 출력버퍼
// pMsg        : 전송할 메시지
// bCRLF       : 문자열 마지막에 '\r\n'을 추가여부
//-----------------------------------------------
int CEmsThread::addMsgToBuf(char * pSendMsgBuf, const char *pMsg, bool bCRLF)
{
	if(pMsg == NULL)
		return 0;
		
	int msgLen = strlen(pMsg);

	strcpy(pSendMsgBuf, pMsg);
	
	if(bCRLF == true){
		strcpy(pSendMsgBuf+msgLen, CRLF);
		msgLen += strlen(CRLF);
	}
	
	return msgLen;
}
					

void CEmsThread::enqRetryProc(shared_ptr<CCpMail> spCpMail, shared_ptr<CEmsClient> spClient)
{
	//Fail Count Increase.
	if((spCpMail.get() == NULL) || (spClient.get() == NULL))
		return;
	if(spClient.get()->m_bRemove == true)
		return;

	spCpMail.get()->incRetryCount();

	if(spCpMail.get()->getStepComplete() == true)
	{
		spClient.get()->RemoveClient();
		return;
	}
	else
	{
		gLog->Write("[iQryUdt_CpMail_SmtpChk7][%s][%s][%s]"
			            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
			            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) );
		gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpChk7);
	}
	
	spClient.get()->RemoveClient();
	spCpMail.get()->setCpMailState(false);

	if(spCpMail.get()->getRetryCount() < m_iMaxRetryCount){
		//shared_ptr<CEmsQueue> spEmsQueue;
		//getEWServerQ(spEmsQueue);
		
		//if(spEmsQueue.get() != NULL){
			spCpMail.get()->setSmtpStep(SMTP_STEP00_Init);
			spCpMail.get()->setTimeStamp();
			//spEmsQueue.get()->addCpMail(spCpMail);
			enqueueMsg(spCpMail);
		//}
	}
}


//-----------------------------------------------
// Thread Main Message Process!!
// pRequest : 서버로부터 전달받아 EmsClient가 가지고있는 Mail 데이터 처리
//-----------------------------------------------
void CEmsThread::processRequest(shared_ptr<CEmsRequest> pRequest)
{
	boost::shared_ptr<CEmsClient> spClient = pRequest->getClient();
	boost::shared_ptr<CCpMail>    spCpMail = pRequest->getClient()->getCpMail();
	if(spCpMail.get() == NULL){
		gLog->Write("[%s][%s][%d] spCpMail is NULL", __FILE__, __FUNCTION__, __LINE__);
		spCpMail.get()->setCpMailState(false);
		return;
	}
	char chFirst3Ch[4]={0,};

	int iCode3Ch = Check_SmtpCode(pRequest, chFirst3Ch);
	spCpMail.get()->setSmtpCode(iCode3Ch);
	if(iCode3Ch == SMTP_SkipOccur)  // -2
	{
		spClient.get()->RemoveClient();
		spCpMail.get()->setCpMailState(false);
		
		if(spCpMail.get()->getStepComplete()==true)
			return;

		if(spCpMail.get()->getRetryCount() < m_iMaxRetryCount){
			spCpMail.get()->setSmtpStep(SMTP_STEP00_Init);
			spCpMail.get()->setTimeStamp();
			
			enqueueMsg(spCpMail);
		}

		gLog->Write("[SMTP_SkipOccur][%s][%s][%s][Insert into Queue(Retry:%d)[CODE:%d]]"
		            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) 
		            , spCpMail.get()->getRetryCount(), iCode3Ch);

		return;
	}

	if(iCode3Ch == SMTP_ErrorOccur) //-1
	{
		spClient.get()->RemoveClient();
		spCpMail.get()->setCpMailState(false);

		if(spCpMail.get()->getStepComplete()==true)
			return;

		if(spCpMail.get()->getRetryCount() < m_iMaxRetryCount){
			spCpMail.get()->setSmtpStep(SMTP_STEP00_Init);
			spCpMail.get()->setTimeStamp();
			
			enqueueMsg(spCpMail);
		}

		gLog->Write("[SMTP_ErrorOccur][%s][%s][%s][Insert into Queue(Retry:%d)[CODE:%d]]"
		            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
		            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN) 
		            , spCpMail.get()->getRetryCount(), iCode3Ch);

		return;
	}
	
	int iSmtp = spCpMail->getSmtpStep();
	
	if(iCode3Ch == RESP_220 && SMTP_STEP01_ConnectEND < iSmtp)
	{
		//[20180105-ischoi
		//HELO 응답을 수신한 이후의 RESP_220 응답은 무시하도록 하자.
	}
	else{
		//switch(spCpMail->getSmtpStep())
		switch(iSmtp)
		{
		case SMTP_STEP00_Init:        //0
			spCpMail.get()->nextSmtpStep();
		case SMTP_STEP01_ConnectEND:  //1
			{
				memset(m_pThreadBuf, '\0', sizeof(m_pThreadBuf));
				//SM_T gLog->Write("T F[CEmsThread::processRequest] D[SMTP_STEP01_ConnectEND] V[%s@%s]", spCpMail->mToId, spCpMail->mToDomain);
				if(getThreadMode() == MODE_M){
	
					if( m_cpMailInfo.get()->_cpSenderDomain != NULL ){
						sprintf(m_pThreadBuf, ENHEAD_HELO " %s%s", m_cpMailInfo.get()->_cpSenderDomain, CRLF );
					}
					else{
						//SenderDomain 이 NULL이므로 값을 넣지 않은 상태로 전송
						sprintf(m_pThreadBuf, ENHEAD_HELO " %s", CRLF );
					}
				}
				else{//if(getThreadMode()==MODE_I)
					const char * cstrSenderDomain = spCpMail->getMailInfo(MHEADER_SENDERDOMAIN);
					if(cstrSenderDomain != NULL){
						sprintf(m_pThreadBuf, ENHEAD_HELO " %s%s", cstrSenderDomain, CRLF );
					}
					else{
						//SenderDomain 이 NULL이므로 값을 넣지 않은 상태로 전송
						sprintf(m_pThreadBuf, ENHEAD_HELO " %s", CRLF );
					}
				}
	
				//HELO 전송시 문제가 발생하게 된다면 응답 코드로 처리하도록 한다.	
				bool bret = spClient->Send((void*)m_pThreadBuf, strlen(m_pThreadBuf));
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail.get()->setSmtpStep(SMTP_STEP02_HeloEND);
			}
			break;
		case SMTP_STEP02_HeloEND:  //2
			{
				spClient->m_IsHelloEnd = true;
	
				//MAIL FROM에는 전송자의 정보가 들어가고 이를 이용하여 메일의  spf를 체크하게 된다.
				if(getThreadMode() == MODE_M){
	
					if(m_cpMailInfo.get()->_cpSenderMail != NULL){
						sprintf(m_pThreadBuf, ENHEAD_MAIL_FROM ":<%s>%s", m_cpMailInfo.get()->_cpSenderMail, CRLF);
					}
					else{
						//m_cpMailInfo.get()->_cpSenderMail 이 NULL일 경우에 대한 에러처리
						sprintf(m_pThreadBuf, ENHEAD_MAIL_FROM ":<NULL>%s", CRLF);
					}
				}
				else{//if(getThreadMode()==MODE_I)
					const char * cstrSenderEmail = spCpMail->getMailInfo(MHEADER_SENDEREMAIL);
					if(cstrSenderEmail != NULL){
						sprintf(m_pThreadBuf, ENHEAD_MAIL_FROM ":<%s>%s", cstrSenderEmail, CRLF );
					}
					else{
						//SenderDomain 이 NULL이므로 값을 넣지 않은 상태로 전송
						sprintf(m_pThreadBuf, ENHEAD_HELO " %s", CRLF );
					}
				}
	
				bool bret = spClient->Send((void*)m_pThreadBuf, strlen(m_pThreadBuf));
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail.get()->setSmtpStep(SMTP_STEP03_MailFromEND);
			}
			break;
		case SMTP_STEP03_MailFromEND:  //3
			{
				memset(m_pThreadBuf, '\0', sizeof(m_pThreadBuf));
				sprintf(m_pThreadBuf, ENHEAD_RCPT_TO ":<%s@%s>%s", spCpMail.get()->getMailInfo(MHEADER_TOID), spCpMail.get()->getMailInfo(MHEADER_TODOMAIN), CRLF );
				
				bool bret = spClient->Send((void*)m_pThreadBuf, strlen(m_pThreadBuf));
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail.get()->setSmtpStep(SMTP_STEP04_RcptToEND);
			}
			break;
		case SMTP_STEP04_RcptToEND:  //4
			{
				memset(m_pThreadBuf, '\0', sizeof(m_pThreadBuf));
				sprintf(m_pThreadBuf, ENHEAD_DATA "%s", CRLF);
				bool bret = spClient->Send((void*)m_pThreadBuf, strlen(m_pThreadBuf));
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail.get()->setSmtpStep(SMTP_STEP05_DataEND);
			}
			break;
		case SMTP_STEP05_DataEND:  //5
			{
				const char *pToName           = spCpMail.get()->getMailInfo(MHEADER_TONAME);
				const char *pContent_Char_Set = getCharSet();   //Set Mail Character Type
	
				//-------------------------------------
				// Only Used variable in MODE_I
				shared_ptr<stSplitInfo> spSplitInfoTT;
				int                     iSplitCntTT     = 0;
				shared_ptr<stSplitInfo> spSplitInfoBody;
				int                     iSplitCntBody   = 0;
				//-------------------------------------
	
				char encode_ToName    [SZ_NAME]= {0,}; 
				char encode_SenderName[SZ_NAME]= {0,};
	
				//Wide String Usage
				bool bUseWstr = false;
				
				if(getThreadMode() == MODE_M){
					//Buffer 초기화
					resetBuffer(spCpMail, MODE_M);
					bUseWstr = getCpMailInfo().get()->_bcpUseWStr;
				}
				else{
					resetBuffer(spCpMail, MODE_I);
					bUseWstr = (atoi(spCpMail->getMailInfo(MHEADER_USEWSTR))==1)?true:false;
				}
				
				if(getThreadMode() == MODE_I){
					preDataProcess(spCpMail, spSplitInfoTT, iSplitCntTT, spSplitInfoBody, iSplitCntBody);
				}
				
				//Wide String
				int iwsCount = 0;
				shared_ptr<stSplitInfo> spwsSplitInfo;
				if(bUseWstr == true){
					spwsSplitInfo = getWideString(spCpMail, iwsCount);
				}
				
				//Auto Increment
				if(m_AutoInc++ < 0){
					m_AutoInc = 0;
				}
			
				//-----------------------------------------
				//SPLIT_ByBody[0] / SPLIT_ByTitle[1] / SPLIT_END[2]
				//-----------------------------------------
				//[Mail Title][WideString]
				//-----------------------------------------
				if((m_pWStrBuf == NULL) || (m_WStrBufLen == 0)){
					//assert(m_pWStrBuf != NULL);
	
					if(m_pWStrBuf != NULL)
						delete [] m_pWStrBuf;
					m_pWStrBuf = NULL;
				
					if( m_EncodeBufLenTT > m_EncodeBufLenBody )
						m_WStrBufLen = m_EncodeBufLenTT;
					else
						m_WStrBufLen = m_EncodeBufLenBody;
				
					if(m_WStrBufLen > 0){
						m_pWStrBuf = new char[m_WStrBufLen];
						memset(m_pWStrBuf, 0, m_WStrBufLen);
					}
				}
	
				//gLog->Write("[%s][%s][%d][SPLIT_ByTitle][Mode:%s]"
				//		            , __FILE__, __FUNCTION__, __LINE__, (getThreadMode() == MODE_M?"M":"I"));
				if(getThreadMode() == MODE_M){
					doWideStrProc(m_pWStrBuf, spCpMail, m_stSplitInfoTT, m_SplitCntTT, spwsSplitInfo, iwsCount, SPLIT_ByTitle);
				}
				else{ //if(getThreadMode() == MODE_I)
					doWideStrProc(m_pWStrBuf, spCpMail, spSplitInfoTT, iSplitCntTT, spwsSplitInfo, iwsCount, SPLIT_ByTitle);
				}
	
				//[Mail Title Encoding]
				int titleLen = strlen(m_pWStrBuf);
	
				memset(m_pTmpBuf, 0, m_TmpBufLen);
				if( titleLen < LEN_Title)
					base64_encode(m_pTmpBuf, m_pWStrBuf, titleLen); 
				else
					base64_encode(m_pTmpBuf, m_pWStrBuf, LEN_Title);
				
				//[Header][Subject]
				sprintf(m_pEncodeBufTT, ENHEAD_SUBJECT ": =?%s?B?%s?=" 
				       , pContent_Char_Set
				       , m_pTmpBuf);
	
				//[Mail Body][WideString]
				//-----------------------------------------
				memset(m_pWStrBuf,       0, m_WStrBufLen);
				memset(m_pEncodeBufBody, 0, m_EncodeBufLenBody);
				
				//gLog->Write("[%s][%s][%d][SPLIT_ByBody][Mode:%s]"
				//		            , __FILE__, __FUNCTION__, __LINE__, (getThreadMode() == MODE_M?"M":"I"));
				if(getThreadMode() == MODE_M){
					doWideStrProc(m_pWStrBuf, spCpMail, m_stSplitInfoBody, m_SplitCntBody, spwsSplitInfo, iwsCount, SPLIT_ByBody);
				}
				else{ //if(getThreadMode() == MODE_I)
					doWideStrProc(m_pWStrBuf, spCpMail, spSplitInfoBody, iSplitCntBody, spwsSplitInfo, iwsCount, SPLIT_ByBody);
				}
	
				//[Mail Body Encoding]
				base64_encode(m_pEncodeBufBody, m_pWStrBuf, strlen(m_pWStrBuf)); 
				//-----------------------------------------
	
				bool bret = false;
				//--- Mail Header 생성 ---------------
				{
					//[Index Time Buffer]
					char pIdxTimeBuffer   [320] = {0,};
					char pHeader_MessageID[320] = {0,};
					char pHeader_Date     [320] = {0,};
					char pHeader_From     [320] = {0,};
					char pHeader_To       [320] = {0,};
					char pHeader_ReplyTo  [320] = {0,}; //REPLY_TO
					
					time_t curTime;
					struct tm* ptTime;
					time(&curTime);
					ptTime = localtime(&curTime);
					
					getMonthName(m_strMonthName, ptTime->tm_mon);
					getWeekName (m_strWeekName,  ptTime->tm_wday);
					//------------------------
					setIndexDate(pIdxTimeBuffer, ptTime, gMessageKey, m_AutoInc);
					//------------------------
					//[Header][Message-ID]
					//------------------------
					memset(m_pTmpBuf, 0, m_TmpBufLen);
					base64_encode(m_pTmpBuf, pIdxTimeBuffer, strlen(pIdxTimeBuffer));
					
					if(getThreadMode() == MODE_M){
						sprintf(pHeader_MessageID, ENHEAD_MESSAGE_ID ": <%s@%s>", m_pTmpBuf, getCpMailInfo().get()->_cpSenderDomain);
					}
					else { //MODE_I
						sprintf(pHeader_MessageID, ENHEAD_MESSAGE_ID ": <%s@%s>", m_pTmpBuf, spCpMail.get()->getMailInfo(MHEADER_SENDERDOMAIN));
					}
					
	
					//[Header][Date]
					sprintf(pHeader_Date, ENHEAD_DATE ": %s, %d %s %d %d:%d:%d +0900"
					                   , m_strWeekName,   ptTime->tm_mday
					                   , m_strMonthName,  ptTime->tm_year + 1900
					                   , ptTime->tm_hour, ptTime->tm_min
					                   , ptTime->tm_sec);
	
					//------------------------
					//[Header][From]
					char       *senderName = NULL;
					const char *senderMail = NULL; 
					
					if(getThreadMode() == MODE_M){
						senderName = getCpMailInfo().get()->_cpSenderName;
						senderMail = getCpMailInfo().get()->_cpSenderMail;
					}
					else { //MODE_I
						senderName = const_cast<char*>(spCpMail.get()->getMailInfo(MHEADER_SENDERNAME));
						senderMail = spCpMail.get()->getMailInfo(MHEADER_SENDEREMAIL);
					}
	
					base64_encode(encode_SenderName, senderName, strlen(senderName));
					
					sprintf( pHeader_From, ENHEAD_FROM ": =?%s?B?%s?= <%s>" 
					       , pContent_Char_Set
						     , encode_SenderName
						     , senderMail);
	
					//------------------------
					//[Header][Reply-To:]
						sprintf( pHeader_ReplyTo, ENHEAD_REPLY_TO ": =?%s?B?%s?= <%s>"
						       , pContent_Char_Set
							     , encode_SenderName
							     , senderMail);
	
					//------------------------
					//[Header][To]
					if(pToName != NULL) //--ischoi ToName이 있을때와 없을때에 대해서 어떻게 처리할지 결정 필요
					{
						RTrim(const_cast<char*>(pToName));
						base64_encode(encode_ToName, const_cast<char*>(pToName), strlen(pToName));
						//---------------------------------------------------------------------------------------------------------------------
						//					//sprintf(m_ThreadBufMini, "From: =?ks_c_5601-1987?B?%s?= <%s>\r\nTo: =?ks_c_5601-1987?B?%s?= <%s@%s>\r\n", 
						//					sprintf(m_ThreadBufMini, "From: =?utf-8?B?%s?= <%s>\r\nTo: =?utf-8?B?%s?= <%s@%s>\r\n", 
						//					                        encode_SenderName, tmpSenderEmail, encode_ToName, tmpToID, tmpToDomain);
						//---------------------------------------------------------------------------------------------------------------------
						sprintf(pHeader_To, ENHEAD_TO ": =?%s?B?%s?= <%s@%s>" 
						                  , pContent_Char_Set
						                  , encode_ToName
						                  , spCpMail.get()->getMailInfo(MHEADER_TOID)
						                  , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN));
					}
					else{
						//---------------------------------------------------------------------------------------------------------------------
						//					//sprintf(m_ThreadBufMini, "From: =?ks_c_5601-1987?B?%s?= <%s>\r\nTo:<%s@%s>\r\n", 
						//					sprintf(m_ThreadBufMini, "From: =?utf-8?B?%s?= <%s>\r\nTo:<%s@%s>\r\n",
						//					                        encode_SenderName, tmpSenderEmail, tmpToID, tmpToDomain);
						//---------------------------------------------------------------------------------------------------------------------
						sprintf(pHeader_To, ENHEAD_TO ": <%s@%s>" 
						                  , spCpMail.get()->getMailInfo(MHEADER_TOID)
						                  , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN));
					}
	
					//---------------------
					//[Header][Content Type Charset]
					char contentTypeCharSet[320] = {0,};
					sprintf(contentTypeCharSet, CONTENT_TYPE ": " CONTENT_TYPE_TXT_HTML "; " CONTENT_CHARSET "=\"%s\""
					       , pContent_Char_Set);
	
	
					//---------------------
					//[Body][Add To Buffer]
					memset(m_pWStrBuf, '\0', m_WStrBufLen);
	
					int iEncodeLen = strlen(m_pEncodeBufBody);
					int copyLen    = 0;
					int CRLFLen    = strlen(CRLF);
	
					for(int iPos = 0; iPos < iEncodeLen;)
					{
						if((iPos + 128) > iEncodeLen)
						{
							memcpy(m_pWStrBuf + copyLen, &(m_pEncodeBufBody[iPos]), iEncodeLen - iPos);
							//strcpy(m_pWStrBuf + copyLen + (iEncodeLen - iPos), CRLF);
							//copyLen += (iEncodeLen - iPos) + CRLFLen;
						}
						else
						{
							memcpy(m_pWStrBuf + copyLen, &(m_pEncodeBufBody[iPos]), 128);
							strcpy(m_pWStrBuf + copyLen + 128, CRLF);
							copyLen += 128 + CRLFLen;
						}
		      
						iPos += 128;
					}
	
					//Make Send Mail Data
					int add_len = 0;
					//------------------------
					//DKIM
					//------------------------
					shared_ptr<CEmsDKIM>  spEmsDKIM;
					CEmsDKIM             *pEmsDKIM         = NULL;
					const char           *cstrSenderDomain = NULL;
	
					if(getThreadMode() == MODE_M){
						if( m_cpMailInfo.get() != NULL ){
							cstrSenderDomain = m_cpMailInfo.get()->_cpSenderDomain;
						}
					}
					else{//if(getThreadMode()==MODE_I)
						if( spCpMail.get() != NULL ){
							cstrSenderDomain = spCpMail.get()->getMailInfo(MHEADER_SENDERDOMAIN);
						}
					}
	
					if(cstrSenderDomain != NULL){
						spEmsDKIM = getEmsDKIM(cstrSenderDomain);
						pEmsDKIM  = spEmsDKIM.get();
					}
					
	
					if(pEmsDKIM != NULL){
						//[JOB ID]
						char jobID[320] = {0,};
						if(getThreadMode() == MODE_M){
							sprintf(jobID, "%s@%s", pIdxTimeBuffer, getCpMailInfo().get()->_cpSenderDomain);
						}
						else { // MODE_I
							sprintf(jobID, "%s@%s", pIdxTimeBuffer, spCpMail.get()->getMailInfo(MHEADER_SENDERDOMAIN));
						}
						//---------------------
						DKIM_STAT status;
						DKIM     *pDKIM = pEmsDKIM->createDKIM_sign(jobID, status); 
						
						//---------------------
						if(pDKIM != NULL){
							bool bState = false;
							int  iCount = 0;
							char hdrbuf[SZ_DEFAULTBUFFER+1]= {'\0',};
							do{
								//---------------------//
								//[DKIM Mail Header]
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, pHeader_From) != DKIM_STAT_OK)      //From
									break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, pHeader_To) != DKIM_STAT_OK)        //To
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, pHeader_Date) != DKIM_STAT_OK)      //Date
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, m_pEncodeBufTT) != DKIM_STAT_OK)    //Subject
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, pHeader_MessageID) != DKIM_STAT_OK) //Message-Id
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, ENHEAD_MIME_VERSION) != DKIM_STAT_OK) //MIME-Version
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, pHeader_ReplyTo) != DKIM_STAT_OK)    //Reply-To
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, contentTypeCharSet) != DKIM_STAT_OK) //Content Character set
									break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, ENHEAD_X_Mailer) != DKIM_STAT_OK)    //X-Mailer
									break;
								iCount++;
								if(pEmsDKIM->setDKIMHeader(pDKIM, CONTENT_TRANSFER_ENCODING) != DKIM_STAT_OK) //Transfer Encoding
									break;
								iCount++;
								if(pEmsDKIM->setDKIMEOH (pDKIM) != DKIM_STAT_OK)
									break;
								//----------------------//
								//[DKIM Mail Body]
								//if(pEmsDKIM->setDKIMBody(pDKIM, m_pEncodeBufBody) != DKIM_STAT_OK) //Mail Body
								iCount++;
								if(pEmsDKIM->setDKIMBody(pDKIM, CRLF) != DKIM_STAT_OK)
									break;
								iCount++;
								if(pEmsDKIM->setDKIMBody(pDKIM, m_pWStrBuf) != DKIM_STAT_OK) //Mail Body
									 break;
								iCount++;
								if(pEmsDKIM->setDKIMBody(pDKIM, CRLF) != DKIM_STAT_OK)
									break;
								iCount++;
								if(pEmsDKIM->setDKIMEOM(pDKIM) != DKIM_STAT_OK)
									break;
								//[Get SignHeader]
								iCount++;
								if(pEmsDKIM->getSignedHeader(pDKIM, hdrbuf, SZ_DEFAULTBUFFER) != DKIM_STAT_OK)
									break;
								iCount++;
								if(pEmsDKIM->closeDKIM(pDKIM) != DKIM_STAT_OK)
									break;
								
								//add_len = addMsgToBuf(m_pThreadBuf+add_len, DKIM_SIGNHEADER ": ", false);
								iCount++;
								add_len = addMsgToBuf(m_pThreadBuf+add_len, hdrbuf);
								//---------------------//
								bState = true;
							}while(false);
							
							if(bState == false)
								gLog->Write("DKIM_Signature Failed[%s][%d]", spCpMail.get()->getWorkerName(), iCount);
						}
					}
	
					//[Header][Add To Buffer]
					add_len += addMsgToBuf(m_pThreadBuf+add_len, pHeader_From);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, pHeader_To);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, pHeader_Date);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, m_pEncodeBufTT);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, pHeader_MessageID);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, ENHEAD_MIME_VERSION);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, pHeader_ReplyTo);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, contentTypeCharSet);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, ENHEAD_X_Mailer);
					add_len += addMsgToBuf(m_pThreadBuf+add_len, CONTENT_TRANSFER_ENCODING);
	
					//Append CRLF
					add_len += addMsgToBuf(m_pThreadBuf+add_len, CRLF);
					//Append Body Data
					add_len += addMsgToBuf(m_pThreadBuf+add_len, m_pWStrBuf);  //[Body Message]
					add_len += addMsgToBuf(m_pThreadBuf+add_len, CRLFx2dotCRLF, false);
	
					int bufLen = strlen(m_pThreadBuf);
	
					int iSendPos=0;
					int iSendSize=0;
					int iRetFailCnt=0;
					
					while(bufLen>iSendPos){
						//bret = spClient->Send((void*)m_pThreadBuf, bufLen);
						//기본 1024크기만큼 전달하고, 
						//마지막 크기가 1024보다 작은 경우 몽땅보낸다.
						if((bufLen-iSendPos) > 1024)
							iSendSize = 1024;
						else{
							iSendSize = bufLen - iSendPos;
						}
						
						bret = spClient->Send((void*)&m_pThreadBuf[iSendPos], iSendSize);
						if(bret==true)
							iSendPos += iSendSize;
						else 
							iRetFailCnt++;
						usleep(1);
					}
					//gLog->Write("[%s][Send Buffer Length:%d][Send RetFailCnt:%d]", __FUNCTION__, bufLen, iRetFailCnt);
				}
	
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail->setSmtpStep(SMTP_STEP06_DataContentEND);
			}
			break;
		case SMTP_STEP06_DataContentEND:  //6
			{
				sprintf(m_pThreadBuf, ENHEAD_QUIT CRLF);
				bool bret = spClient->Send((void*)m_pThreadBuf, strlen(m_pThreadBuf));
	      //spCpMail->setSmtpStep(SMTP_STEP07_QuitOrRsetEnd); //Next Process
				if(!bret){
					enqRetryProc(spCpMail, spClient);
				}
				else
					spCpMail->setSmtpStep(SMTP_STEP07_QuitOrRsetEnd);
			}
			break;
		case SMTP_STEP07_QuitOrRsetEnd:  //7
			{
				spCpMail->setStepComplete(true);
				spClient.get()->RemoveClient();
			}
			break;
		default : 
				//gLog->Write("[%s][%s][%d][MSG Unconfirmed Step][MSG:%d]", __FILE__, __FUNCTION__, __LINE__, spCpMail->getSmtpStep());
				gLog->Write("[MSG Unconfirmed Step][%s][%s][%s][STEP:%d]"
				            , spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
				            , spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)
				            , spCpMail.get()->getMailInfo(MHEADER_TODOMAIN)
				            , spCpMail.get()->getSmtpStep() );
		}
	}


	spCpMail.get()->setCpMailState(false);
}
