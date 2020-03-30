#ifndef __EMS_RABBIT_MESSAGE_QUEUE_MANAGER_HEADER__
#define __EMS_RABBIT_MESSAGE_QUEUE_MANAGER_HEADER__

#include "../NetTemplate/LockObject.h"
#include "EmsCommon.h"
#include "EmsRMQConnect.h"

//Rabbitmq Channel을 관리하기위한 클래스
class CRMQChannel{
	public:
		 CRMQChannel();
		~CRMQChannel();
};

using namespace boost;
using namespace std;

class CRMQSet;

typedef  unordered_map<string, boost::shared_ptr<CEmsRMQConnect> >   mapRMQConnections; //Rabbitmq 연결 관리 map


//EmsRMQManager에서 접속에 대한 관리를 처리
class CEmsRMQManager : public LockObject
{
	public:
		             ~CEmsRMQManager();
		
		static CEmsRMQManager *getInstance();
		
		bool             setRMQManager(const char* rmq_ip, int rmq_port, const char *rmq_user, const char *rmq_passwd, const char *rmq_vhost); // RMQ Manager 설정
		void             resetRMQConInfo();
		void             reset        ();
		//Connect Rabbit Message Queue
		shared_ptr<AMQP> connectRMQ  ();
		
		AMQPQueue       *getExistQueue(const char *exname, const char *name);
		AMQPQueue       *getQueue     (const char *exname, const char *qname, bool bIsCMDSender = false);
		AMQPExchange    *getExchange  (const char *exname, const char *qname, bool bIsCMDSender = false);
		
		//CMD Publisher
		bool             setDefaultExHeader (AMQPExchange* ex);           // Set Header Information
		
		//Create RMQ Connection
		shared_ptr<CEmsRMQConnect> createRMQConn(const char *exname, const char* route_key, const char *name, short param = 0 );

		//Delete Queue & Channel
		bool             deleteRMQConn(const char* exname, const char* qname, bool bState=false);  // Queue를 RMQConnect 관리 리스트에서 삭제
		
		//RMQSet Functions
		shared_ptr<CRMQSet> getRMQSet (string exname);  //Exchange Name에 해당하는  CRMQSet을 리턴


		//Thread Flags
		bool         isStop        ()            { return m_bIsStop; }
    void         setStopState  (bool bState) { m_bIsStop  = bState; }
		bool         isRun         ()            { return m_bIsRun; }
		void         setRunState   (bool bState) { m_bIsRun   = bState; }
		bool         isCMDUsable   ()            { return m_bIsCMDQUsable; }
		void         setCMDState   (bool bState) { m_bIsCMDQUsable = bState; }
		
	private: 
		             CEmsRMQManager();
		
	private:
		static CEmsRMQManager *m_pEmsRMQManager;
		
		//RMQ Configurations
		int                m_iRMQPort;
		char               m_chRMQIPAddr[SZ_IP4ADDRESS+1];
		char               m_chRMQUser  [SZ_NAME+1];
		char               m_chRMQPasswd[SZ_PASSWD+1];
		char               m_chRMQVHost [SZ_NAME+1];
		//Thread Flags
		bool               m_bIsStop;
		bool               m_bIsRun;
		bool               m_bIsCMDQUsable;
		pthread_t          m_RMQConnectChecker;
					
		mapRMQConnections  m_mapRMQConnections;       // (Mail) Queue(campaign)이름을 기준으로하는 Rabbitmq 연결 관리
		mapRMQConnections  m_mapRMQCMDConnections;    // (CMD)
		mapRMQConnections  m_mapRMQLOGConnections;    // (LOG)
		mapRMQConnections  m_mapRMQCommonConnections; // (COMMON)

};


#define theEmsRMQManager() CEmsRMQManager::getInstance()
	
class CRMQSet { 
	
	public:
		CRMQSet (const char *name) 
		: m_Name(name), m_pAMQPEx(NULL)
		{	}
		
		~CRMQSet() {
			if(m_spAMQP.get() != NULL)
				m_spAMQP.get()->closeChannel();
		}
		
		const char      *getName         ()                        { return    m_Name.c_str(); }
		void             setAMQP         (shared_ptr<AMQP> spAMQP) { m_spAMQP  = spAMQP;       }
		shared_ptr<AMQP> getAMQP         ()                        { return    m_spAMQP;       }
		void             setAMQPExchange (AMQPExchange *pAMQPEx)   { m_pAMQPEx = pAMQPEx;      }
		AMQPExchange    *getAMQPExchange ()                        { return    m_pAMQPEx;      }
		
	private:
		CRMQSet () {}
	
	private:
		string           m_Name;
		shared_ptr<AMQP> m_spAMQP;
		AMQPExchange    *m_pAMQPEx;
};

	
#endif  //__EMS_RABBIT_MESSAGE_QUEUE_MANAGER_HEADER__


