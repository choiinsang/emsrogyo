
#include "EmsConfig.h"
#include "EmsMX.h"
#include "EmsLog.h"

extern CEmsLog * gLog;

/* Error variables */
extern int h_errno;  /* for resolver errors */
extern int errno;    /* general system errors */
/****************************************************************
 * nsError -- Print an error message from h_errno for a failure *
 *     looking up NS records.  res_query(  ) converts the DNS   *
 *     message return code to a smaller list of errors and      *
 *     places the error value in h_errno.  There is a routine   *
 *     called herror(  ) for printing out strings from h_errno  *
 *     like perror(  ) does for errno.  Unfortunately, the      *
 *     herror(  ) messages assume you are looking up address    *
 *     records for hosts.  In this program, we are looking up   *
 *     NS records for zones, so we need our own list of error   *
 *     strings.                                                 *
 ****************************************************************/
void nsError(int error, char *domain)
{
    switch(error){
        case HOST_NOT_FOUND:
          gLog->Write("Unknown zone [%s][ERROR CODE:%d]", domain, error);
          break;
        case NO_DATA:
          gLog->Write("No NS records for [%s][ERROR CODE:%d]", domain, error);
          break;
        case TRY_AGAIN:
          gLog->Write("No response for NS query [%s][ERROR CODE:%d]", domain, error);
          break;
        default:
          gLog->Write("Unexpected error[ERROR CODE:%d]", error);
          break;
    }
}

/****************************************************************
 * returnCodeError -- print out an error message from a DNS     *
 *     response return code.                                    *
 ****************************************************************/
void returnCodeError(int rcode, char *nameserver)
{
    gLog->Write("[NameServer][%s]: ", nameserver);
    switch(rcode){
        case ns_r_formerr:
          gLog->Write("FORMERR response");
          break;
        case ns_r_servfail:
          gLog->Write("SERVFAIL response");
          break;
        case ns_r_nxdomain:
          gLog->Write("NXDOMAIN response");
          break;
        case ns_r_notimpl:
          gLog->Write("NOTIMP response");
          break;
        case ns_r_refused:
          gLog->Write("REFUSED response");
          break;
        default:
          gLog->Write("unexpected return code");
          break;
    }
}


/****************************************************************
 * MXCompare -- Compare MXRecord pref values to qsort MXRecord list   *
 *     of the MXRecord vector.                                    *
 ****************************************************************/
//--------------------------------------------------
//MXList 정렬을 위한 비교함수
int MXCompare(const void *A, const void *B)
{
	int a =   (*(boost::shared_ptr<_MXRecord>  *)A)->_ns_pref;
	int b =   (*(boost::shared_ptr<_MXRecord>  *)B)->_ns_pref;

   if (a < b)
      return -1;
   else if (a > b)
      return  1;
   else
      return  0;
}


/****************************************************************
 *   CEmsMX Class Value
****************************************************************/
//public
CEmsMX::CEmsMX(const char * dname)
: m_bRemove (false)
{
	setDomainName(dname);
	setBuildTime();
	//setLastUsedTime();
	//m_iMaxRetryCount = theEMSConfig()->getMAXMXRetryCount();
}

CEmsMX::~CEmsMX()
{
	clearMXInfoList();
}


const string CEmsMX::getDomainName()
{ 
	return m_sDomainName;
};

bool CEmsMX::setDomainName(const char* dname)
{ 
	if(dname != NULL)
	{
		m_sDomainName = string(dname);
		return true;
	}
	else
		return false;
}

bool CEmsMX::setNSName(string &target, const char* dname)
{	
	if(dname != NULL)
	{
		target = string(dname);
		return true;
	}	
	else 
		return false;
};


//-----------------------------------------------------
// MX의 IPAddress의 Retry Count를 증가시킨다
//-----------------------------------------------------
void CEmsMX::incRetryCount  (const char *pIPAaddr )
{
	//순환문으로 IP를 찾는닫
	//해당 IP의 _ns_retrycount
	vecMXList          *pMXList = this->getMXList();
	vecMXList::iterator itr_mx  = pMXList->begin();
	for(;itr_mx != pMXList->end(); itr_mx++){
		if(strcmp(pIPAaddr, itr_mx->get()->_ns_addr)==0){
			itr_mx->get()->_ns_retrycount++;
			break;
		}
	}
}


void CEmsMX::clearMXInfoList(){
	vecMXList * theMXList = this->getMXList();
	theMXList->clear();
}


//--------------------------------------------------
// MX HostName을  IP로 변경하여 저장
bool CEmsMX::HostnameToIP(char *ipbuf, const char *hostname)
{
	bool retval = false;
	if((hostname == NULL) || (strlen(hostname)==0) || (ipbuf == NULL)){
		return retval;
	}
	else{
		
		struct hostent *he;
		struct in_addr a;
		he = gethostbyname(hostname);

		if(he == NULL)
		{
			return retval;
		}
	
		while (*he->h_addr_list)
		{
	
			bcopy(*he->h_addr_list++, (char *) &a, sizeof(a));
	
			if(strlen(inet_ntoa(a)) > SZ_IP4ADDRESS)
			{
				break;
			}
	
			strcpy(ipbuf, inet_ntoa(a));
		}
		retval = true;
	}
	
	return retval;
}


//--------------------------------------------------
// MX Record List를 구성한다
//--------------------------------------------------
int CEmsMX::MXLookup(const char *domainName)  // MXList 를 재구성
{
	unsigned char response[SZ_DEFAULTBUFFER] = {0,};

	int           responseLen         = 0;
	int           resourceRecordCount = 0;
	
	ns_msg        msgHandle;
	ns_rr         resourceRecord;
	
	vecMXList * theMXList = this->getMXList();
	theMXList->clear();
	m_iMXCount = 0;
	
	if ( (responseLen = res_query(domainName, ns_c_in, ns_t_mx, response, SZ_DEFAULTBUFFER)) > 0 ) {
		bool bIsGabia = false;
		
		if(strcmp(domainName, DEFAULT_DOMAIN_GABIA_COM) == 0)
			bIsGabia = true;
			
		if (ns_initparse(response, responseLen, &msgHandle) < 0 ) {
			gLog->Write("[%s][%s][%d] ns_initparse: %s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
			return -1;                   // MXLookup 실패시 오류코드 필요할 것임.현재 방치중.... <TODO Log-ischoi>
		}
		else {

      if(ns_msg_getflag(msgHandle, ns_f_rcode) != ns_r_noerror){
          returnCodeError((int)ns_msg_getflag(msgHandle, ns_f_rcode), const_cast<char*>(domainName) );
          return -1;                     // MXLookup 실패시 오류코드 필요할 것임.현재 방치중.... <TODO Log-ischoi>
      }
			
			char  tmpNSName[NS_MAXDNAME] = {0,};
			resourceRecordCount = ns_msg_count(msgHandle, ns_s_an);
			
			for(int i = 0; i < resourceRecordCount; i++)
			{
				if (ns_parserr(&msgHandle, ns_s_an, i, &resourceRecord) == 0 && ns_rr_type(resourceRecord) == ns_t_mx) {

					boost::shared_ptr<_MXRecord>  tmpspMXInfo = boost::shared_ptr<_MXRecord>(new _MXRecord);
					_MXRecord * tmpMXInfo = tmpspMXInfo.get();
					
					memset(tmpNSName, '\0', NS_MAXDNAME);

					tmpMXInfo->_ns_pref = ns_get16(ns_rr_rdata(resourceRecord));
					ns_name_uncompress(ns_msg_base(msgHandle), ns_msg_end(msgHandle), ns_rr_rdata(resourceRecord) + NS_INT16SZ, tmpNSName, NS_MAXDNAME);
					
					if(setNSName(tmpMXInfo->_ns_name, tmpNSName) == false) {
						continue;
					}

					if(HostnameToIP(tmpMXInfo->_ns_addr, tmpNSName) == true){
						if(bIsGabia ==  true){
							if(strcmp(tmpMXInfo->_ns_addr, "121.254.168.215")== 0)
								continue;
						}
						
						theMXList->push_back(tmpspMXInfo);
						m_iMXCount++;
						if(m_iMXCount >= MAX_MX_HOST_COUNT) //MX HostIP는 MAX_MX_HOST_COUNT 개수 까지만 관리한다
							break;
					}
		    }
		  }

	  	if (m_iMXCount > 1){ // MXList Sorting 
			  qsort(&theMXList->at(0), m_iMXCount, sizeof(theMXList->at(0)), MXCompare);
			}
		}
		setBuildTime();  //갱신 시간 설정
	}
	else{		  /* res_query 실패한 경우 */
		//res_query Failed
		boost::shared_ptr<_MXRecord>  tmpspMXInfo = boost::shared_ptr<_MXRecord>(new _MXRecord);
		_MXRecord * tmpMXInfo = tmpspMXInfo.get();
		
		tmpMXInfo->_ns_pref = 0;
		
		if(setNSName(tmpMXInfo->_ns_name, domainName) == false) {
			return -1;
		}

		if(HostnameToIP(tmpMXInfo->_ns_addr, domainName) == true){
			theMXList->push_back(tmpspMXInfo);
			m_iMXCount++;
		}
	}
	
	if(m_iMXCount == 0){
		boost::shared_ptr<_MXRecord>  tmpspMXInfo = boost::shared_ptr<_MXRecord>(new _MXRecord);
		_MXRecord * tmpMXInfo = tmpspMXInfo.get();
		
		tmpMXInfo->_ns_pref = 0;
		
		if(setNSName(tmpMXInfo->_ns_name, domainName) == false) {
			return -1;
		}

		if(HostnameToIP(tmpMXInfo->_ns_addr, domainName) == true){
			theMXList->push_back(tmpspMXInfo);
			m_iMXCount++;
		}
	}
	
	return 0;
}

//--------------------------------------------------
// BaseTime과 Current Time의 시간 간격이 intervalTime 보다 큰 경우 true 아닌 경우 false
//--------------------------------------------------
bool CEmsMX::compareTime(time_t baseTime, int intervalTime){
	time_t currtime;
	time(&currtime);
	int retval = difftime(currtime, baseTime);
	
	if(retval > intervalTime)  
		return true;
	else
		return false;	
}


bool CEmsMX::getMXIPAddress(char *pmxipaddr, int interval, int iLimitConCnt)
{
	//interval이 되지 않았거나 
	//Connection Count가 오버 되었을 경우에는 return false
	
	vecMXList * tmpMXList = this->getMXList();
	vecMXList::iterator itr_mx = tmpMXList->begin();

	if(itr_mx != tmpMXList->end()){
		boost::shared_ptr<_MXRecord> tmpmxRecord = *itr_mx;
		int curTimeDiff, preTimeDiff;
		struct timeval CurrTime;
		gettimeofday(&CurrTime, NULL);

		if(tmpmxRecord.get() == NULL){
			setRemove(true);
			return false;
		}
		
		preTimeDiff = ( CurrTime.tv_sec - tmpmxRecord.get()->_lastConnTime.tv_sec ) * 1000 +
		            ( CurrTime.tv_usec - tmpmxRecord.get()->_lastConnTime.tv_usec ) / 1000;

		for(itr_mx = tmpMXList->begin();itr_mx != tmpMXList->end(); itr_mx++){
			if(itr_mx->get()->_iConnCnt < iLimitConCnt){
				//&&(itr_mx->get()->_ns_retrycount < m_iMaxRetryCount)){
				curTimeDiff = ( CurrTime.tv_sec - itr_mx->get()->_lastConnTime.tv_sec ) * 1000 +
				              ( CurrTime.tv_usec - itr_mx->get()->_lastConnTime.tv_usec ) / 1000;
				if(curTimeDiff >= interval){
				 	tmpmxRecord = *itr_mx;
				 	preTimeDiff = curTimeDiff;
				 	break;
				}
			}
		}

		if(tmpmxRecord.get()->_iConnCnt >= iLimitConCnt){
			return false;
		}
							              
		if(preTimeDiff < interval)
			return false;
			
		memcpy(pmxipaddr, tmpmxRecord.get()->_ns_addr, strlen(tmpmxRecord.get()->_ns_addr));
		tmpmxRecord.get()->_iConnCnt++;
		gettimeofday(&tmpmxRecord.get()->_lastConnTime, NULL);
		return true;
	}

	return true;
}

void CEmsMX::decConnCount(const char *ipaddr )
{
	vecMXList * tmpMXList = this->getMXList();
	vecMXList::iterator itr_mx = tmpMXList->begin();
		
	for(;itr_mx != tmpMXList->end(); itr_mx++){
		if(strcmp(ipaddr, itr_mx->get()->_ns_addr)==0){
			if(itr_mx->get()->_iConnCnt < 1){
				itr_mx->get()->_iConnCnt=0;
			}
			else{
				//gLog->Write("[IPDISCONN][IP:%s][_iConnCnt :%d][_ns_retrycount:%d]  ", ipaddr, itr_mx->get()->_iConnCnt, itr_mx->get()->_ns_retrycount);
				itr_mx->get()->_iConnCnt--;
			}
			break;
		}
	}
}
