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
// MX Information  생성 -ischoi
//-------------------------------------------------

//--------------------------------------------------
// MX 서버 호스트 IP 정보
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
		bool        HostnameToIP   (char *ipbuf, const char *hostname); // hostname을 ip로 변경
		int         MXLookup       (const char *domainName);
		
		bool        compareTime    (time_t baseTime, int intervalTime); /*baseTime과 현재시간의 IntervalTime 비교. intervalTime보다큰 경우 true*/
		
		time_t      getMXBuildTime (){ return  m_tBuildTime;    }

		int         getMXCount     (){ return  m_iMXCount;      }
		void        incRetryCount  (const char *ipaddr ); //{ ipaddr해당하는 IP _ns_retrycount 증가시키고,  m_iRetryCount++;         }
		
		void        decConnCount   (const char *ipaddr ); //{ ipaddr해당하는 접속 카운트 다운 }
				
		vecMXList * getMXList      (){ return &m_MXRecordList;}
		void        clearMXInfoList();  // MX Recode List 전부 삭제
		
		bool        getMXIPAddress (char *mxipaddr, int interval, int iLimitConCnt);// 사용 가능한 IP 주소를 리턴
		
		void        setRemove      (bool bState){ m_bRemove = bState; }  // MX 삭제 플래그 설정
		bool        isRemove       (){ return m_bRemove; }  // MX 갱신을 위해 삭제여부 리턴

	private:
		            CEmsMX         ();
		void        setBuildTime   (){ time(&m_tBuildTime);  }

	private:
		std::string m_sDomainName;
			
		time_t      m_tBuildTime;    // MX Record 생성된 시간
		
		int         m_iMXCount;      // ns server count
		//int         m_iMaxRetryCount;// Max send fail retry count 
		
		vecMXList   m_MXRecordList;  // MX Record List
		bool        m_bRemove;
};

#endif  // __CEMS_MX_CLASS_HEADER__
