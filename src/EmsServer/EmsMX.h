#ifndef __CEMS_MX_CLASS_HEADER__
#define __CEMS_MX_CLASS_HEADER___

#include <iostream>
#include <string.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <errno.h>
#include <netdb.h>

#include "EmsCommon.h"
#include "EmsDefine.h"

using namespace std;
using namespace boost;

//------------------------------------------------
// MX Information  ���� -ischoi
//-------------------------------------------------

//--------------------------------------------------
// MX ���� ȣ��Ʈ IP ����
struct _MXRecord {

	_MXRecord(){
		_ns_pref       = 0;
		_ns_retrycount = 0;
		_iConnCnt      = 0;
		memset(_ns_addr, '\0', SZ_IP4ADDRESS+1);
		gettimeofday(&_lastConnTime, NULL);
		_lastConnTime.tv_sec -= 1 ;
	};
	
	~_MXRecord(){
	};
	
	
	std::string    _ns_name;
	char           _ns_addr[SZ_IP4ADDRESS+1];
	int            _ns_pref;
	int            _ns_retrycount;
	int            _iConnCnt;
	struct timeval _lastConnTime;
	
};

typedef  boost::shared_ptr<_MXRecord>  sptr_MX;
typedef  std::vector<sptr_MX >         vecMXList;


class CEmsMX 
{
	public:
		            CEmsMX(const char* dname);
		            ~CEmsMX();
		
		const std::string getDomainName();
			
		bool        setDomainName  (const char *dname);
		bool        setNSName      (std::string &target, const char *dname);
		bool        HostnameToIP   (char *ipbuf, const char *hostname); // hostname�� ip�� ����
		int         MXLookup       (const char *domainName);
		
		bool        compareTime    (time_t baseTime, int intervalTime); /*baseTime�� ����ð��� IntervalTime ��. intervalTime����ū ��� true*/
		
		time_t      getMXBuildTime (){ return  m_tBuildTime;    }

		int         getMXCount     (){ return  m_iMXCount;      }
		void        incRetryCount  (const char *ipaddr ); //{ ipaddr�ش��ϴ� IP _ns_retrycount ������Ű��,  m_iRetryCount++;         }
		
		void        decConnCount   (const char *ipaddr ); //{ ipaddr�ش��ϴ� ���� ī��Ʈ �ٿ� }
				
		vecMXList * getMXList      (){ return &m_MXRecordList;}
		void        clearMXInfoList();  // MX Recode List ���� ����
		
		bool        getMXIPAddress (char *mxipaddr, int interval, int iLimitConCnt);// ��� ������ IP �ּҸ� ����
		
		void        setRemove      (bool bState){ m_bRemove = bState; }  // MX ���� �÷��� ����
		bool        isRemove       (){ return m_bRemove; }  // MX ������ ���� �������� ����

	private:
		            CEmsMX         ();
		void        setBuildTime   (){ time(&m_tBuildTime);  }

	private:
		std::string m_sDomainName;
			
		time_t      m_tBuildTime;    // MX Record ������ �ð�
		
		int         m_iMXCount;      // ns server count
		//int         m_iMaxRetryCount;// Max send fail retry count 
		
		vecMXList   m_MXRecordList;  // MX Record List
		bool        m_bRemove;
};

#endif  // __CEMS_MX_CLASS_HEADER__
