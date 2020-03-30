//##############################################################################################
#include "EmsClient.h"

//#include "sth_syslog.h"
#include "EmsLog.h"
#include "EmsDefineString.h"


#if _USE_DOMAINLIST_
	#include "EmsDomainListSet.h"
	extern CEmsDomainListSet *gDomainList;
#endif //_USE_DOMAINLIST_

extern CEmsLog * gLog;



//-------------------------------------
// CEmsClient 
//-------------------------------------
CEmsClient::CEmsClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
: CClientEpoll(ulKey, iMaxReadBuffer, iMaxWriteBuffer, epoll_fd, pRemoveList)
, m_iMode (MODE_M)
{
}

//-------------------------------------
// ~CEmsClient 
//-------------------------------------
CEmsClient::~CEmsClient()
{
	
#if _USE_DOMAINLIST_
	if(m_spCCpMail.get() != NULL){
		
		
		const char *pDomainName = m_spCCpMail.get()->getMailInfo(MHEADER_TODOMAIN);
		const char *pMailIndex  = m_spCCpMail.get()->getMailInfo(MHEADER_MAILNUMBER);
		const char *pId         = m_spCCpMail.get()->getMailInfo(MHEADER_TOID);
		gDomainList->disconnClient(pDomainName, m_spCCpMail.get()->getConnIP());
		gLog->Write("[Close][%s][%s][%s]", pMailIndex, pId, pDomainName);
	}
#endif //_USE_DOMAINLIST_
}


//-------------------------------------
// Reset CpMail Shared_ptr Object
//-------------------------------------
void CEmsClient::resetCpMail()
{
	m_spCCpMail.reset();
}


//-------------------------------------
// Set CpMail Shared_ptr Object
//-------------------------------------
void CEmsClient::setCpMail (shared_ptr<CCpMail> l_CpMail)
{
	m_spCCpMail = l_CpMail;
}
