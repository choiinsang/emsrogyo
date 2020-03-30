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
		
		int                   getFromDomainList(const char *dname, std::vector<string> &vecDomainList); // ������ ����Ʈ ������ �о� ���Ϳ� ����

		//bool makeMXListWithDomainName(const char *dname);    // ������ �̸����� MXList ����
		bool                  InsertMX         (const char *dname, int &errCode, string &errStr);  // ������ �̸����� MX ����Ʈ ���� ����(CEmsMX �����Ͽ� ����)
		bool                  DeleteMX         (const char *dname, int &errCode);  // ����
		bool                  RefreshMX        (const char *dname, int &errCode);  // ����
		
		//Thread���� MX List ��������
		bool                  CheckMXListState ();          // MX List�� �ֱ������� �����Ͽ� �����ϰų� ����
		void                  StartCheckThread ();          // MX List ������ ���� ������ ����
		static void          *ChkThreadFunc    (void * Param); // ������ ���� �Լ�
		
		
		shared_ptr<CEmsMX>    getMXFromMXList  (const char *dname, int &errCode, string &errStr);    // mapMXList�κ���  CEmsMX�� ���� 			
		mapMXList            *getMXList        ();     // MXList ����
		
		bool                  checkUsable      (const char *pDName, int interval_msec);
		void                  closeDomainConnIP(const char *pDName, const char *pIPAddr);
		
	private:
		                      CEmsMXManager ();  //singleton ������
		bool                  DeleteMXAll   ();  // ���λ���

	private:
		static CEmsMXManager *m_pEmsMXManager;
		       mapMXList     *m_pMapMXList;
		       pthread_t      m_CheckThread;
		       int            m_ThreadState;  // ready:0, run:1, stop:2
		       int            m_iMaxRetryCount;
	
};

#define theMXManager()  CEmsMXManager::getInstance()
	
#endif  //__CEMS_MX_MANAGER_HEADER__
