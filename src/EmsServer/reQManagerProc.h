#ifndef __RE_ENQUEUE_RABBITMQ_MANAGER_HEADER__
#define __RE_ENQUEUE_RABBITMQ_MANAGER_HEADER__

#define MENU_EXIT           -1
#define MENU_FILE_TO_QUEUE   1
#define MENU_QUEUE_TO_FILE   2
#define MENU_DELETE_QUEUE    3


#include "EmsLog.h"
#include "EmsRMQManager.h"

using namespace std;

class CReQManagerProc {

		CReQManagerProc();

	public:
		~CReQManagerProc();
		
		static CReQManagerProc *getInstance();
		
		void showMenu   ();
		
		void strLower   (char *strSrc);
		void showLine   ();
		int  parseMenu  (char *strMenu);
		bool procMenu   (int   iMenu);
		int  getData    (char *pBuffer);

	private:
		
		void                  procEnQueueFromFiles    ();
		void                  procGetFilesFromQueue   ();
		shared_ptr<CEmsQueue> procGetMessagesFromDir  (const char *pDirName);
		shared_ptr<CEmsQueue> procGetMessagesFromQueue(const char *pExchange, const char *pQName);
		void                  procSendToQueue         (const char *pExchange, const char *pQName, shared_ptr<CEmsQueue> spEmsQueue);
		void                  procSendToDir           (const char *pDirName,  shared_ptr<CEmsQueue> spEmsQueue);
		

	private:
		static CReQManagerProc *m_pReQManager;
		
		int    m_iManual_HELP_Cnt;
		int    m_iManual_MENU_Cnt;
		int    m_iManual_CMD_Cnt;
		int    m_iManual_BASE_Cnt;		
	
};

#define theReQManager()  CReQManagerProc::getInstance()
#endif  //__RE_ENQUEUE_RABBITMQ_MANAGER_HEADER__

