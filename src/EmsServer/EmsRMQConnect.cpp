//##############################################################################################
#include <string.h>
#include "EmsRMQConnect.h"
//#include "EmsLog.h"
//
//extern CEmsLog * gLog;

//#####[ CEmsRMQConnect()  ]############################
CEmsRMQConnect::CEmsRMQConnect()
:	m_pAMQPQueue (NULL)
, m_pAMQPEx    (NULL)
{
}


//#####[ ~CEmsRMQConnect ]###########################
// Rabbitmq ¼Ò¸êÀÚ ¼³Á¤
CEmsRMQConnect::~CEmsRMQConnect()
{
	if(m_spAMQP.get() != NULL)
		m_spAMQP.get()->closeChannel();
}

