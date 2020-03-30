//------------------------------------------------------------------------
// Title : EMS Manager
// Author: ischoi
// PP    : Mx Information Management
// Date  : 20150224
//------------------------------------------------------------------------

#ifndef __CEMS_MX_MANAGER_HEADER__
#define __CEMS_MX_MANAGER_HEADER__

#include <string.h>
#include <pthread.h>
#include <boost/shared_ptr.hpp>

#ifdef __USETR1LIB__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map
#endif

#include "../NetTemplate/LockObject.h"

#include "EmsMX.h"

using namespace boost;

typedef shared_ptr<CEmsMX>              spEMSMX;
typedef unordered_map< string, spEMSMX> mapMXList;

class CEmsMXManager : public LockObject
{
	public:
		                     ~CEmsMXManager    ();
		static CEmsMXManager *getInstance      ();
		
		int                   getFromDomainList(const char *dname, std::vector<string> &vecDomainList); // 도메인 리스트 파일을 읽어 벡터에 저장

		//bool makeMXListWithDomainName(const char *dname);    // 도메인 이름으로 MXList 생성
		bool                  InsertMX         (const char *dname, int &errCode, string &errStr);  // 도메인 이름으로 MX 리스트 생성 삽입(CEmsMX 생성하여 삽입)
		bool                  DeleteMX         (const char *dname, int &errCode);  // 삭제
		bool                  RefreshMX        (const char *dname, int &errCode);  // 갱신
		
		//Thread에서 MX List 관리실행
		bool                  CheckMXListState ();          // MX List를 주기적으로 관리하여 갱신하거나 삭제
		void                  StartCheckThread ();          // MX List 관리를 위한 쓰레드 실행
		static void          *ChkThreadFunc    (void * Param); // 쓰레드 실행 함수
		
		
		shared_ptr<CEmsMX>    getMXFromMXList  (const char *dname, int &errCode, string &errStr);    // mapMXList로부터  CEmsMX를 전달 			
		mapMXList            *getMXList        ();     // MXList 전달
		
		bool                  checkUsable      (const char *pDName, int interval_msec);
		void                  closeDomainConnIP(const char *pDName, const char *pIPAddr);
		
	private:
		                      CEmsMXManager ();  //singleton 생성자
		bool                  DeleteMXAll   ();  // 전부삭제

	private:
		static CEmsMXManager *m_pEmsMXManager;
		       mapMXList     *m_pMapMXList;
		       pthread_t      m_CheckThread;
		       int            m_ThreadState;  // ready:0, run:1, stop:2
		       int            m_iMaxRetryCount;
	
};

#define theMXManager()  CEmsMXManager::getInstance()
	
#endif  //__CEMS_MX_MANAGER_HEADER__
