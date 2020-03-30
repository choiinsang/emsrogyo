//**************************************
//* Specific Domain List Management Implamentation.
//* Set Different Connection Count and Delay Time.
//* by ischoi
//* date : 2015. 10. x
//**************************************/
#ifndef __CEMS_DOMAINLISTSET_HEADER__
#define __CEMS_DOMAINLISTSET_HEADER__

#include "LockObject.h"
#include "DefineNetGlobal.h"
#include "EmsCommon.h"

using namespace boost;
using namespace std;

//struct domain information
typedef struct str_domain_info_t{
	str_domain_info_t(int delay=DEFAULT_INTERVAL_MSEC, int maxCon=1){
		_delay      =delay;
		_maxCon     =maxCon;
		_curConCount=0;
	};
	
	void incrConnCount(){ _curConCount++; };
	void decrConnCount(){ if( (_curConCount--) < 0 )
		                      _curConCount=0; 
		                  };
	bool isUsable     (){
		                    if(_maxCon == 0)
		                    	return true;
		                    else if(_maxCon > _curConCount){
		                    	incrConnCount();
		                    	return true;
		                    }
		                    return false;
		                  };

	void setDelay     (int delay)  { _delay = delay;  };
	int  getDelay     ()           { return _delay;   };
	void setMaxCon    (int maxCon) { _maxCon = maxCon;};
	int  getMaxCon    ()           { return _maxCon;  };
	
	timeval *getLastTime() { return &_lastSendTime; };

private:
	struct timeval _lastSendTime;
	int            _delay;
	int            _maxCon;
	int            _curConCount;
	
}strDomainInfo;

//Domain List Set Class
class CEmsDomainListSet : public LockObject
{
	public:

		virtual ~CEmsDomainListSet();
		
		static   CEmsDomainListSet *getInstance();

		//Set DomainList Info
		bool         setDomainListInfo    (const char *fpath);
		const char * checkDuplicateDNS    (const char *pDnsName);

		//Get Send Interval
		bool         getDomainInterval    (const char *pDomainName, int &interval, int &conCnt);
		//Check Domain Connect Usable or Not
		bool         isConnectDomainUsable(const char *pDomainName, int &intarval);
		void         disconnClient        (const char *pDomainName, const char *pIPAddr);
		
		void         printListInfo        ();
	private:
		      CEmsDomainListSet();

	private:
		static CEmsDomainListSet                          *m_pEmsDomainListSet;
		shared_ptr< unordered_map<string, strDomainInfo> > m_spMapDomainListInfo;
		unordered_map<string, string>                      m_mapDuplicateDNS;
};

#define theDomainList() CEmsDomainListSet::getInstance()

#endif  //__CEMS_DOMAINLISTSET_HEADER__
