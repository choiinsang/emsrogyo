//##############################################################################################
#include "EmsQueue.h"
#include "EmsDefineString.h"

//-------------------------------------
// CCpMail - Mail Instance
//-------------------------------------
CCpMail::CCpMail()
: m_Time           (0)
, m_bCpMailState   (false)
, m_iMode          (MODE_M)
, m_TryCnt         (0)
, m_iSmtpCode      (0)
, m_iSmtpStep      (SMTP_STEP00_Init)      //SMTP_STEP00_Init
, m_iExpResultCode (0)
, m_IsStepComplete (false)	
{
}


CCpMail::~CCpMail()
{
	unordered_map<string, char*>::iterator itr_umap = m_mapCpMailInfo.begin();
	char *tmpCh = NULL;
	for(;itr_umap != m_mapCpMailInfo.end(); itr_umap++){
		tmpCh = (char *)itr_umap->second;
		if(tmpCh != NULL){
			free(tmpCh);
			itr_umap->second = NULL;
		}
	}
	m_mapCpMailInfo.clear();
}


bool CCpMail::setMailInfo(const char * hType, const char * hData)
{
	bool retval = false;
	if( (hType != NULL) && (hData != NULL) ){
		char *pData = strdup(hData);
		m_mapCpMailInfo.insert(std::pair<string, char *>(string(hType), pData ) );
		retval = true;
	}
	return retval;
}


const char * CCpMail::getMailInfo (const char * hType)
{
	const char * pData = NULL;
	if(hType !=  NULL){
		unordered_map<string, char *>::iterator itr_map = m_mapCpMailInfo.find(string(hType));
		if(itr_map != m_mapCpMailInfo.end()){
			pData = (char *)itr_map->second;
		}
	}	
	return pData;
}


void CCpMail::setTimeStamp()
{
	time(&m_Time);
}

void CCpMail::setConnIP(const char *ipaddr)
{
	memset(m_connIPAddr, 0, SZ_IP4ADDRESS+1);
	strncpy(m_connIPAddr, ipaddr, SZ_IP4ADDRESS);
}

//------------------------------------------------------
// CEmsQueue 
//------------------------------------------------------
CEmsQueue::CEmsQueue()
{	
}

//------------------------------------------------------
// ~CEmsQueue 
//------------------------------------------------------
CEmsQueue::~CEmsQueue()
{
}

//------------------------------------------------------
// CpMail Queue에 메일 메시지를 입력
int CEmsQueue::addCpMail(boost::shared_ptr<CCpMail> spCpMail)
{//메일 정보를 Rabbitmq로부터 Queue에 넣는다.
	Lock();
	m_deqMailQueue.push_back(spCpMail);
	Unlock();
	return 0;
}

//------------------------------------------------------
// CpMail Queue에 들어있는 메시지를 꺼냄
shared_ptr<CCpMail>  CEmsQueue::getCpMail()
{
	shared_ptr<CCpMail> tmpCpMail;
	Lock();
	if(m_deqMailQueue.size() > 0){
		tmpCpMail = m_deqMailQueue.front();
		m_deqMailQueue.pop_front();
	}
	Unlock();
	return tmpCpMail;
}

