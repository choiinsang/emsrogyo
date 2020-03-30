//##############################################################################################
#ifndef __CEMS_CLIENT_HEADER__
#define __CEMS_CLIENT_HEADER__

#include <string.h>
#include "packet_header.h"
#include "packets.h"
#include "CClientEpoll.h"
#include "CMultiServerEpoll.h"
#include "CRequest.h"
#include "CRemoveList.h"
#include "EmsQueue.h"

using namespace boost;
using namespace std;

class CEmsClient : public CClientEpoll
{
public:
	             CEmsClient     (unsigned long ulKey
	                            ,int iMaxReadBuffer
	                            ,int iMaxWriteBuffer
	                            ,int epoll
	                            ,boost::shared_ptr<CRemoveList <unsigned long> > pRemoveList);
	virtual     ~CEmsClient     ();

	void         setMode        (int iMode)               { m_iMode = iMode; };
	int          getMode        ()                        { return  m_iMode; };
	
	void         setCampaignNoKey (std::string campaignNoKey) { m_strEmsCampaignNoKey = campaignNoKey; };
	std::string  getCampaignNoKey ()                        { return m_strEmsCampaignNoKey; };

	void                 resetCpMail();
	void                 setCpMail  (boost::shared_ptr<CCpMail> l_CpMail);
	shared_ptr<CCpMail>  getCpMail  () { return m_spCCpMail; };
	
	void         setClientState(int iState);
	int          getClientState()  { return m_iClientState; }
	
private:
	int                  m_iMode;
	shared_ptr<CCpMail>  m_spCCpMail;
	std::string          m_strEmsCampaignNoKey;
	int                  m_iClientState;

};

#endif  //__CEMS_CLIENT_HEADER__

