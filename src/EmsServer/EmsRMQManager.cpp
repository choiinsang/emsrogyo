//#pragma once
#include "EmsConfig.h"
#include "EmsRMQManager.h"
#include "EmsLog.h"

extern CEmsLog * gLog;
//--------------------------------
// fprintf()->EmsLog ����Ͽ� ó����
//--------------------------------

CEmsRMQManager::CEmsRMQManager()
: m_iRMQPort     (0)
, m_bIsStop      (false)
, m_bIsRun       (false)
, m_bIsCMDQUsable(false)
{
	memset(m_chRMQIPAddr, 0, SZ_IP4ADDRESS+1);
	memset(m_chRMQUser,   0, SZ_NAME+1);
	memset(m_chRMQPasswd, 0, SZ_PASSWD+1);
	memset(m_chRMQVHost,  0, SZ_NAME+1);
}

CEmsRMQManager::~CEmsRMQManager()
{
	if(!isStop()){ //Running State
		m_bIsStop = true;
	}
}

CEmsRMQManager * CEmsRMQManager::m_pEmsRMQManager = NULL;
	
//---------------------------------------
// theEmsRMQManager Instance
//---------------------------------------
CEmsRMQManager * CEmsRMQManager::getInstance(){
	if(m_pEmsRMQManager == NULL){

		m_pEmsRMQManager = new CEmsRMQManager();

		stRabbitmqInfo * pRabbitmqInfo = theEMSConfig()->getRabbitmqInfo();

		m_pEmsRMQManager->setRMQManager(pRabbitmqInfo->_chRabbitmq_IP
		                              , pRabbitmqInfo->_iRabbitmq_Port
		                              , pRabbitmqInfo->_chRabbitmq_User
		                              , pRabbitmqInfo->_chRabbitmq_Passwd
		                              , pRabbitmqInfo->_chRabbitmq_Vhost );
	}
	return m_pEmsRMQManager;
}



//---------------------------------------
//theEmsRMQManager reset RMQ Connection Info!
//---------------------------------------
void CEmsRMQManager::resetRMQConInfo()
{
	try{
		if(m_pEmsRMQManager == NULL){
			getInstance();
		}
		else{
			theEMSConfig()->nextRabbitmqInfo();
			stRabbitmqInfo *pRabbitmqInfo = theEMSConfig()->getRabbitmqInfo();
			m_pEmsRMQManager->setRMQManager(pRabbitmqInfo->_chRabbitmq_IP
			                              , pRabbitmqInfo->_iRabbitmq_Port
			                              , pRabbitmqInfo->_chRabbitmq_User
			                              , pRabbitmqInfo->_chRabbitmq_Passwd
			                              , pRabbitmqInfo->_chRabbitmq_Vhost );
		}
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][EmsRMQManager Connection Info Reset Error Occurred.]", __FILE__, __FUNCTION__, __LINE__);
	}
}


//---------------------------------------
//theEmsRMQManager reset!
//---------------------------------------
void CEmsRMQManager::reset()
{
	try{
		if(m_pEmsRMQManager == NULL){
			getInstance();
		}
		else{
			theEMSConfig()->nextRabbitmqInfo();
			stRabbitmqInfo *pRabbitmqInfo = theEMSConfig()->getRabbitmqInfo();
			m_pEmsRMQManager->setRMQManager(pRabbitmqInfo->_chRabbitmq_IP
			                              , pRabbitmqInfo->_iRabbitmq_Port
			                              , pRabbitmqInfo->_chRabbitmq_User
			                              , pRabbitmqInfo->_chRabbitmq_Passwd
			                              , pRabbitmqInfo->_chRabbitmq_Vhost );
		}
		
		gLog->Write("[%s][%s][%d][1:%d][2:%d][3:%d][4:%d]"
		           , __FILE__, __FUNCTION__, __LINE__
		           , m_mapRMQConnections.size()
		           , m_mapRMQCMDConnections.size()
		           , m_mapRMQLOGConnections.size()
		           , m_mapRMQCommonConnections.size()
		           );
		
		if(m_mapRMQConnections.size() > 0)
			m_mapRMQConnections.clear();
		if(m_mapRMQCMDConnections.size() > 0)
			m_mapRMQCMDConnections.clear();
		if(m_mapRMQLOGConnections.size() > 0)
			m_mapRMQLOGConnections.clear();
		if(m_mapRMQCommonConnections.size() > 0)
			m_mapRMQCommonConnections.clear();

		gLog->Write("[%s][%s][%d][EmsRMQManager Reset OK]", __FILE__, __FUNCTION__, __LINE__);
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][EmsRMQManager Reset Error]", __FILE__, __FUNCTION__, __LINE__);
	}
}


//#####[ CEmsRMQManager ]###################################
//---------------------------------------
// ������ �Է� ���� ���� ��� RMQ Manager�� �������� �ʵ��� �Ѵ�.
//---------------------------------------
bool CEmsRMQManager::setRMQManager(const char* rmq_ip, int rmq_port, const char *rmq_user, const char *rmq_passwd, const char *rmq_vhost)
{
	bool retval = false;

	do{
		if(rmq_ip == NULL)
			break;
		if(rmq_port < 0)
			break;
		if(rmq_user == NULL)
			break;
		if(rmq_passwd == NULL)
			break;
			
		retval = true;
	}while(0);
	
	if(!retval){
		gLog->Write("[%s][%s][%d][ERROR][Rabbitmq Configuration Set Failed][%s][%d][%s][%s][%s]"
		             , __FILE__, __FUNCTION__, __LINE__, rmq_ip, rmq_port, rmq_user, rmq_passwd, rmq_vhost);
		return false;
	}

	// ���� IPv4 �ּ� ü�踦 ����Ѵٴ� �����Ʒ� �ڵ���.
	// ���� IPv6 �ּ� ü�谡 ��� �� ��� �ݵ�� �����۾� �Ǿ�� ��.
	if(strlen(rmq_ip) > SZ_IP4ADDRESS){
		gLog->Write("[%s][%s][%d][ERROR][IP ADDRESS ERROR][IP:%s]", __FILE__, __FUNCTION__, __LINE__, rmq_ip);
		return false;
	}

	Lock();
	try{
		memset(m_chRMQIPAddr, 0, SZ_IP4ADDRESS +1);
		memset(m_chRMQUser,   0, SZ_NAME       +1);
		memset(m_chRMQPasswd, 0, SZ_PASSWD     +1);
		memset(m_chRMQVHost,  0, SZ_NAME       +1);
		
		memcpy(m_chRMQIPAddr, rmq_ip,     SZ_IP4ADDRESS);
		memcpy(m_chRMQUser,   rmq_user,   SZ_NAME);
		memcpy(m_chRMQPasswd, rmq_passwd, SZ_PASSWD);
		
		if(rmq_vhost != NULL)
			memcpy(m_chRMQVHost, rmq_vhost, SZ_NAME);
	
		m_iRMQPort = rmq_port;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][Set RMQ Connection Info Failed]", __FILE__, __FUNCTION__, __LINE__);	
	}

	Unlock();
	return true;
}


//---------------------------------------
// AMQP  ���� 
//---------------------------------------
shared_ptr<AMQP> CEmsRMQManager::connectRMQ ()
{
	shared_ptr<AMQP> tmpspAMQP;

	try{
		char strrmqcfg[SZ_TMPBUFFER] = {0,};
		sprintf(strrmqcfg, "%s:%s@%s:%d/%s", m_chRMQPasswd, m_chRMQUser, m_chRMQIPAddr, m_iRMQPort, m_chRMQVHost);
		tmpspAMQP = shared_ptr<AMQP>(new AMQP(strrmqcfg));
		return tmpspAMQP;
	}
	catch(AMQPException ex){
		//gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
	}
	catch(...){
		//gLog->Write("[%s][%s][%d][ERROR] AMQP Create Failed", __FILE__, __FUNCTION__, __LINE__);
	}
	
	return tmpspAMQP;

}

//---------------------------------------
// CEmsRMQConnect  ��ü ���� ����
//---------------------------------------
shared_ptr<CEmsRMQConnect> CEmsRMQManager::createRMQConn(const char *exname, const char* route_key, const char *name, short param )
{
	shared_ptr<CEmsRMQConnect> tmpspRMQConn = shared_ptr<CEmsRMQConnect>(new CEmsRMQConnect);
	shared_ptr<CEmsRMQConnect> tmpspRMQNULL;

	if(tmpspRMQConn.get() != NULL){

		shared_ptr<AMQP> tmpspAMQP = connectRMQ();
		if(tmpspAMQP.get() != NULL){  //AMQP is Not NULL
				
			tmpspRMQConn.get()->setAMQP(tmpspAMQP);
			//Exchange ����
			AMQPExchange * tmpEx = tmpspAMQP.get()->createExchange();
			if(tmpEx != NULL){
				tmpEx->Declare(exname, "direct"); 
				tmpspRMQConn.get()->setRMQExchange(tmpEx);
			}
			else{
				gLog->Write("[%s][%s][%d][ERROR][CreateExchange Failed!][%s][%s][%s]", __FILE__, __FUNCTION__, __LINE__, exname, route_key, name);
			}
			
			if(((strcmp(route_key, RMQ_ROUTE_KEY_CMD)== 0)||(strcmp(route_key, RMQ_ROUTE_KEY_EA_CMD)== 0))
				 && (param != (AMQP_EXCLUSIVE|AMQP_AUTODELETE))){
		 		//gLog->Write("[%s][%s][%d][10][%d][%d]!", __FILE__, __FUNCTION__, __LINE__, param, (AMQP_EXCLUSIVE|AMQP_AUTODELETE));
				return tmpspRMQConn;
			}
			else{
				// Queue ����
				AMQPQueue *tmpQueue = tmpspAMQP.get()->createQueue(name);
				if(tmpQueue != NULL){
					tmpQueue->Declare(name,  param);
					tmpQueue->Bind   (exname, route_key);
					tmpspRMQConn.get()->setRMQueue(tmpQueue);
					
					return tmpspRMQConn;
				}
				else {
					return tmpspRMQNULL;
				}
			}
		}
		else{
			//gLog->Write("[%s][%s][%d][%s][%s][%s][Get RMQ Connection Failed!]", __FILE__, __FUNCTION__, __LINE__, exname, route_key, name);
			return tmpspRMQNULL;
		}
	}
	else {
		return tmpspRMQNULL;
	}
}


//---------------------------------------
// Exchange Name�� �ش��ϴ� (AMQPQueue *)�� ����
//---------------------------------------
AMQPQueue * CEmsRMQManager::getExistQueue (const char *exname, const char *name)
{
	AMQPQueue         *tmpQueue           = NULL;
	mapRMQConnections *ptmpRMQConnections = NULL;

	if(strcmp(exname, RMQ_EXCHANGE_MAIL)==0){
		ptmpRMQConnections = &m_mapRMQConnections;
	}
	else if((strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0) || (strcmp(exname, RMQ_EXCHANGE_CMD)==0)){  //CMD 
		ptmpRMQConnections = &m_mapRMQCMDConnections;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_LOG)==0){  //LOG
		ptmpRMQConnections = &m_mapRMQLOGConnections;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_COMMON)==0){
		ptmpRMQConnections = &m_mapRMQCommonConnections;
	}
	else{
		return tmpQueue;
	}

	mapRMQConnections::iterator itr_q = ptmpRMQConnections->find(name);
	
	if(itr_q != ptmpRMQConnections->end()){ 
		//�ش� RMQConnection�� �ִٸ�
		shared_ptr<CEmsRMQConnect> tmpspRMQConn = (shared_ptr<CEmsRMQConnect>)itr_q->second;
		
		if(tmpspRMQConn.get() != NULL){
			tmpQueue = tmpspRMQConn.get()->getRMQueue();
		}
	}

	return tmpQueue;
}


//---------------------------------------
// Exchange Name�� �ش��ϴ� (AMQPQueue *)�� ����/���ٸ� ����
//---------------------------------------
AMQPQueue * CEmsRMQManager::getQueue (const char *exname, const char *name, bool bIsCMDSender)
{
	AMQPQueue         *tmpQueue           = NULL;
	mapRMQConnections *ptmpRMQConnections = NULL;
	short              param              = AMQP_DURABLE;
	const char        *route_key          = name;

	gLog->Write("[%s][EXCHANGE:%s][QUEUENAME:%s]", __FUNCTION__, exname, name);
	
	if(strcmp(exname, RMQ_EXCHANGE_MAIL)==0){
		ptmpRMQConnections = &m_mapRMQConnections;
	}
	else if((strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0) || (strcmp(exname, RMQ_EXCHANGE_CMD)==0)){  //CMD 
		ptmpRMQConnections = &m_mapRMQCMDConnections;
		
		if(bIsCMDSender == false){
			param = AMQP_EXCLUSIVE|AMQP_AUTODELETE;
		}
		
		if(strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0)
			route_key = RMQ_ROUTE_KEY_EA_CMD;
		else //if(strcmp(exname, RMQ_EXCHANGE_CMD)==0)
			route_key = RMQ_ROUTE_KEY_CMD;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_LOG)==0){  //LOG
		ptmpRMQConnections = &m_mapRMQLOGConnections;
		route_key          = RMQ_ROUTE_KEY_LOG;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_COMMON)==0){
		ptmpRMQConnections = &m_mapRMQCommonConnections;
		route_key          = name;
	}
	else{
		return tmpQueue;
	}

	mapRMQConnections::iterator itr_q = ptmpRMQConnections->find(name);
	
	if(itr_q != ptmpRMQConnections->end()){ 
	//�ش� RMQConnection�� �ִٸ�
		
		shared_ptr<CEmsRMQConnect> tmpspRMQConn = (shared_ptr<CEmsRMQConnect>)itr_q->second;
		
		if(tmpspRMQConn.get() != NULL){

			tmpQueue = tmpspRMQConn.get()->getRMQueue();
	
			if(tmpQueue == NULL){	//RMQConnect�� �ִµ� ťä���� ���ٸ�

				shared_ptr<AMQP> spAMQP = tmpspRMQConn->getAMQP();
				if(spAMQP.get() == NULL){  //AMQP is NULL

					shared_ptr<AMQP> tmpspAMQP = connectRMQ();				
					if(tmpspAMQP.get() != NULL){
						
						tmpspRMQConn.get()->setAMQP(tmpspAMQP);
						//Exchange ����
						AMQPExchange * tmpEx = tmpspAMQP.get()->createExchange();
						if(tmpEx != NULL){
							tmpEx->Declare(exname, "direct"); 
							tmpspRMQConn.get()->setRMQExchange(tmpEx);
						}
						else{
							gLog->Write("[%s][%s][%d][ERROR][%s][%s] createExchange Failed!", __FILE__, __FUNCTION__, __LINE__, exname, name);
							return NULL;
						}
					}
					else{
						gLog->Write("[%s][%s][%d][ERROR][%s][%s] Get RMQ Connection Failed!", __FILE__, __FUNCTION__, __LINE__, exname, name);
						return NULL;
					}
				}
	
				if(spAMQP.get() != NULL){
					// Queue ����
					tmpQueue = spAMQP.get()->createQueue(name);
					if(tmpQueue != NULL){
						tmpQueue->Declare(name,   param);
						tmpQueue->Bind   (exname, route_key);
						tmpspRMQConn.get()->setRMQueue(tmpQueue);
					}
					else{
						return NULL;
					}
				}
				else{
					return NULL;
				}
			}
			else{
				return tmpQueue;
			}
		}
		else { //if(tmpspRMQConn.get() == NULL)
			shared_ptr<CEmsRMQConnect> tmpspRMQConn = createRMQConn(exname, route_key, name, param);
			
			if(tmpspRMQConn.get() != NULL){
				ptmpRMQConnections->insert(std::make_pair<std::string, shared_ptr<CEmsRMQConnect> >(name, tmpspRMQConn));
				tmpQueue = tmpspRMQConn.get()->getRMQueue();
			}
			else {
				gLog->Write("[%s][%s][ERROR][%d][%s][%s][CEmsRMQConnect Return NULL]"
				            , __FILE__, __FUNCTION__, __LINE__, exname, name);
			}
		}
	}
	else{
		//�ش� RMQConnection�� ���ٸ�
		//gLog->Write("[%s][%s][%d][RMQConnection is NULL][%s][%s][%s]", __FILE__, __FUNCTION__, __LINE__, exname, route_key, name);
		shared_ptr<CEmsRMQConnect> tmpspRMQConn = createRMQConn(exname, route_key, name, param);
		
		if(tmpspRMQConn.get() != NULL){
			ptmpRMQConnections->insert(std::make_pair<std::string, shared_ptr<CEmsRMQConnect> >(name, tmpspRMQConn));
			tmpQueue = tmpspRMQConn.get()->getRMQueue();
		}
		else {
		}
	}

	return tmpQueue;
}


//---------------------------------------
// Exchange Name�� �ش��ϴ� (AMQPExchange *)�� ����
//---------------------------------------
AMQPExchange * CEmsRMQManager::getExchange (const char *exname, const char *name, bool bIsCMDSender)
{
	AMQPExchange      *tmpEx              = NULL;
	mapRMQConnections *ptmpRMQConnections = NULL;
	short              param              = AMQP_DURABLE;
	const char        *route_key          = name;

	gLog->Write("[exchange : %s][name : %s] ", exname, name);
	if(strcmp(exname, RMQ_EXCHANGE_MAIL)==0){
		ptmpRMQConnections = &m_mapRMQConnections;
	}
	else if((strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0) || (strcmp(exname, RMQ_EXCHANGE_CMD)==0)){  // CMD 
		ptmpRMQConnections = &m_mapRMQCMDConnections;
		param              = AMQP_EXCLUSIVE|AMQP_AUTODELETE;

		if(strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0)
			route_key = RMQ_ROUTE_KEY_EA_CMD;
		else //if(strcmp(exname, RMQ_EXCHANGE_CMD)==0)
			route_key = RMQ_ROUTE_KEY_CMD;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_LOG)==0){  // LOG
		ptmpRMQConnections = &m_mapRMQLOGConnections;
		route_key          = RMQ_ROUTE_KEY_LOG;
	}
	else if(strcmp(exname, RMQ_EXCHANGE_COMMON)==0){
		ptmpRMQConnections = &m_mapRMQCommonConnections;
		route_key          = name;
	}
	else{
		return tmpEx;
	}
	
//-----------------------------------------------
// ���� CMD �� ��� Queue�� ������ ���ƾ��Ѵ�.(CMD Queue�� �����Ǹ�..�������) 
//-----------------------------------------------

	mapRMQConnections::iterator itr_q = ptmpRMQConnections->find(name);
		
	if(itr_q != ptmpRMQConnections->end()){ 	//�ش� RMQConnection�� �ִٸ�
		
		shared_ptr<CEmsRMQConnect> tmpspRMQConn = (shared_ptr<CEmsRMQConnect>)itr_q->second;
		
		if(tmpspRMQConn.get() != NULL){
			tmpEx = tmpspRMQConn.get()->getRMQExchange();
			if(tmpEx == NULL){
				// Exchange�� ���ٸ� Exchange�� ����������
				shared_ptr<AMQP> tmpspAMQP = tmpspRMQConn.get()->getAMQP();

				if(tmpspAMQP.get() == NULL){
					//RMQConnection �ٽ� �����ؼ� ��������
					ptmpRMQConnections->erase(itr_q);
					
					shared_ptr<CEmsRMQConnect> tmpspRMQConn_new;
					if(bIsCMDSender == true){
						tmpspRMQConn_new = createRMQConn(exname, route_key, name);
					}
					else{
						tmpspRMQConn_new = createRMQConn(exname, route_key, name, param);
					}
	
					if(tmpspRMQConn_new.get() != NULL){
						ptmpRMQConnections->insert(std::make_pair<std::string, shared_ptr<CEmsRMQConnect> >(name, tmpspRMQConn_new));
						tmpEx = tmpspRMQConn_new.get()->getRMQExchange();
					}
					return tmpEx;
				}
				else{
					tmpEx = tmpspAMQP.get()->createExchange();
					if(tmpEx != NULL){
						tmpEx->Declare(exname, "direct");
						tmpspRMQConn.get()->setRMQExchange(tmpEx);
					}
					return tmpEx;
				}
			}
			else {
				return tmpEx;
			}
		}
		else { //RMQConnection is NULL: �����ؼ� ����������	
			shared_ptr<CEmsRMQConnect> tmpspRMQConn;
			if(bIsCMDSender == true){
				 tmpspRMQConn = createRMQConn(exname, route_key, name);
			}
			else{
				tmpspRMQConn = createRMQConn(exname, route_key, name, param);
			}

			if(tmpspRMQConn.get() != NULL){
				ptmpRMQConnections->insert(std::make_pair<std::string, shared_ptr<CEmsRMQConnect> >(name, tmpspRMQConn));
				tmpEx = tmpspRMQConn.get()->getRMQExchange();
				return tmpEx;
			}
			else{
				gLog->Write("[%s][%s][%d]Create CEmsRMQConnect Error Occurred!", __FILE__, __FUNCTION__, __LINE__);
				return NULL;
			}
		}
	}
	else{ //�ش� RMQConnection�� ���ٸ�
		shared_ptr<CEmsRMQConnect> tmpspRMQConn;
		if(bIsCMDSender == true){
			 tmpspRMQConn = createRMQConn(exname, route_key, name);
		}
		else{
			tmpspRMQConn = createRMQConn(exname, route_key, name, param);
		}

		if(tmpspRMQConn.get() != NULL){
			ptmpRMQConnections->insert(std::make_pair<std::string, shared_ptr<CEmsRMQConnect> >(name, tmpspRMQConn));
			tmpEx = tmpspRMQConn.get()->getRMQExchange();
			return tmpEx;
		}
		else{
			gLog->Write("[%s][%s][%d][ERROR][Create CEmsRMQConnect Error Occurred!]", __FILE__, __FUNCTION__, __LINE__);
			resetRMQConInfo();
			return NULL;
		}
	}
	return tmpEx;
}


//---------------------------------------
// Exchange�� ���� CMD ���� Header ����
//---------------------------------------
bool CEmsRMQManager::setDefaultExHeader(AMQPExchange *ex){
	bool retval = false;
	if(ex != NULL){
		ex->setHeader("Delivery-mode", 2);
    ex->setHeader("Content-type", "text/plain");
    retval = true;
	}

	return retval;
}


//---------------------------------------
// Queue�� ����Ʈ���� ����
//---------------------------------------
bool CEmsRMQManager::deleteRMQConn(const char* exname, const char* name, bool bState)
{
	try{
		if(bState == true){
			AMQPQueue * tmpQueue = getExistQueue(exname, name);
			if(tmpQueue != NULL)
				tmpQueue->Delete(name);
		}
		
		mapRMQConnections * ptmpRMQConnections = NULL;
	
		if(strcmp(exname, RMQ_EXCHANGE_MAIL)==0){        //MAIL
			ptmpRMQConnections = &m_mapRMQConnections;
		}
		else if((strcmp(exname, RMQ_EXCHANGE_CMD)==0) ||(strcmp(exname, RMQ_EXCHANGE_EA_CMD)==0)){  // CMD 
			ptmpRMQConnections = &m_mapRMQCMDConnections;
		}
		else if(strcmp(exname, RMQ_EXCHANGE_LOG)==0){    // LOG
			ptmpRMQConnections = &m_mapRMQLOGConnections;
		}
		else if(strcmp(exname, RMQ_EXCHANGE_COMMON)==0){ //COMMON
			ptmpRMQConnections = &m_mapRMQCommonConnections;
		}
		else{
			return false;
		}
	
		mapRMQConnections::iterator itr_rmqc = ptmpRMQConnections->find(name);
	
		if(itr_rmqc != ptmpRMQConnections->end()){
			gLog->Write("[DELETE Channel][Name:%s]", name);
			ptmpRMQConnections->erase(itr_rmqc);
		}
		return true;
	}
	catch(AMQPException ex){
		//ischoi -- ����  gLog�� ��ü�ϵ��� �Ѵ�
		gLog->Write("[%s][%s][%d][AMQPException Msg:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
	}
	catch(...){
		gLog->Write("[%s][%s][%d][Connection Delete Error Occurred!]", __FILE__, __FUNCTION__, __LINE__);
	}
	
	return false;
}



