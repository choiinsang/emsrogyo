#ifndef __CEMS_RABBITMQ_CONNECTOR_HEADER__
#define __CEMS_RABBITMQ_CONNECTOR_HEADER__


#include <boost/shared_ptr.hpp>
#include <pthread.h>
#include "EmsDefine.h"
#include "amqpcpp-master/include/AMQPcpp.h"

using namespace boost;
//Rabbitmq�� �����ϴ� �κи� ����
//EmsRMQManager���� ���ӿ� ���� ������ ó���� ����
//������� �����̾����� Exchange�� �����Ͽ� ó����.
class CEmsRMQConnect
{
public:

	 CEmsRMQConnect(); // �⺻ ������
	~CEmsRMQConnect(); // �Ҹ���
	
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
