#ifndef __CEMS_RABBITMQ_CONNECTOR_HEADER__
#define __CEMS_RABBITMQ_CONNECTOR_HEADER__


#include <boost/shared_ptr.hpp>
#include <pthread.h>
#include "EmsDefine.h"
#include "amqpcpp-master/include/AMQPcpp.h"

using namespace boost;
//Rabbitmq에 접속하는 부분만 구현
//EmsRMQManager에서 접속에 대한 관리를 처리할 것임
//연결관리 목적이었으나 Exchange만 설정하여 처리함.
class CEmsRMQConnect
{
public:

	 CEmsRMQConnect(); // 기본 생성자
	~CEmsRMQConnect(); // 소멸자
	
	shared_ptr<AMQP> getAMQP       () { return m_spAMQP; };
	void             setAMQP       (shared_ptr<AMQP> spAMQP) { m_spAMQP = spAMQP; };

	void             setRMQueue    (AMQPQueue *pQueue)       { m_pAMQPQueue = pQueue;};  // Set Rabbitmq Queue
	AMQPQueue       *getRMQueue    ()                        { return m_pAMQPQueue; };   // Get Rabbitmq Queue

	void             setRMQExchange(AMQPExchange *pExchange) { m_pAMQPEx = pExchange;};  // Set Rabbitmq Exchange
	AMQPExchange    *getRMQExchange()                        { return m_pAMQPEx; };      // Get Rabbitmq Exchange

private: 
	
	shared_ptr<AMQP> m_spAMQP;
	AMQPQueue       *m_pAMQPQueue;
	AMQPExchange    *m_pAMQPEx;

};

#endif  //__CEMS_RABBITMQ_CONNECTOR_HEADER__
