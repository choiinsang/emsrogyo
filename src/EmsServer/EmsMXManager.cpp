
#include "EmsConfig.h"
#include "EmsMXManager.h"
#include "EmsLog.h"
#include "EmsDomainListSet.h"

extern CEmsLog * gLog;

CEmsMXManager * CEmsMXManager::m_pEmsMXManager=NULL;
//public 
CEmsMXManager * CEmsMXManager::getInstance()
{
	if(m_pEmsMXManager == NULL){
		m_pEmsMXManager =  new CEmsMXManager();
	}
	return m_pEmsMXManager;
}


CEmsMXManager::CEmsMXManager()
	:m_ThreadState(0)
{
	m_pMapMXList = new mapMXList;
	StartCheckThread();
};


CEmsMXManager::~CEmsMXManager()
{
	//m_CheckThread Thread �����Ű��
	m_ThreadState = 2; //Stop State
	pthread_join(m_CheckThread, NULL);
	
	if(m_pMapMXList != NULL){
		DeleteMXAll();
		delete m_pMapMXList;
	}
	
	m_pMapMXList = NULL;
};

bool CEmsMXManager::DeleteMX (const char * dname, int &errCode){ 
	return true;
}

bool CEmsMXManager::RefreshMX(const char * dname, int &errCode){
	return true;
}


//-----------------------------------------------------
// MxManager Thread���� MX ����Ʈ�� �ֱ������� ����ó��
//-----------------------------------------------------
bool CEmsMXManager::CheckMXListState(){

	mapMXList * pMXList = this->getMXList();

	if(pMXList == (mapMXList *)NULL){
		gLog->Write("[%s][%s][%d]MX List is NULL!", __FILE__, __FUNCTION__, __LINE__);
	}

	if(pMXList->size() <= 0){
		return true;
	}
	
	//int     iHoldingTime   = theEMSConfig()->getDNSHoldingTime();
	int     iRefreshTime   = theEMSConfig()->getDNSRefreshTime();
	
	bool    bRemoveDNS ;
	bool    bRefresh   ;
	spEMSMX tmpEmsMX   ;
	mapMXList::iterator itr_map = pMXList->begin();
	
	for(;itr_map != pMXList->end(); ){
		bRemoveDNS = false;
		bRefresh   = false;
		tmpEmsMX   = itr_map->second;

		//1.���� �ð��� ���� ���
		if( tmpEmsMX.get()->compareTime(tmpEmsMX.get()->getMXBuildTime(), iRefreshTime) == true ){
			//gLog->Write("[2][XM][ERASE][%s][iRefreshTime:%d]", itr_map->first.c_str(), iRefreshTime); 
			itr_map = pMXList->erase(itr_map);
			bRefresh = true;
			continue;
		}
		//2.Retry Count ���� ���
		else if( tmpEmsMX.get()->isRemove() == true ){
			//gLog->Write( "[%s][%s][%d] Remove Domain MX[%s]", __FILE__, __FUNCTION__, __LINE__, itr_map->first.c_str());
			itr_map = pMXList->erase(itr_map);
			bRefresh = true;
			continue;
		}
		//etc.
		else{
			//Do nothing
		}
		
		itr_map++;
	}
	return true;
}

void CEmsMXManager::StartCheckThread(){
	m_ThreadState = 1; //Run State
	pthread_create(&m_CheckThread, NULL, &ChkThreadFunc, this);
}

//-----------------------------------------------
// ������ �����Լ�
//-----------------------------------------------
void *CEmsMXManager::ChkThreadFunc(void * Param){
	CEmsMXManager * l_mxmanager = (CEmsMXManager *)Param;
	
	while( true )
	{
		if(l_mxmanager->m_ThreadState == 1){  //Run State
			l_mxmanager->CheckMXListState();
			usleep(THREAD_SLEEPTIME_1SEC); 
		}
		else if(l_mxmanager->m_ThreadState == 2) //Stop State
			break;
	}
	
	return (void *)NULL;
}

//-----------------------------------------------
// MX List�� �����͸� ���� �����ϵ��� ��
//-----------------------------------------------
bool CEmsMXManager::DeleteMXAll(){
	if( (m_pMapMXList != NULL) && (m_pMapMXList->size() > 0)){
		m_pMapMXList->clear();
	}
	
	return true;
}

//-----------------------------------------------
// Trim ���� ���� ���� Function���� �и��Ͽ� ���
//-----------------------------------------------
const char * DELIMITERS = " \t,;\n";

const char * trim_chars(char * chbuf, const char * l_delimiters){
	if(chbuf ==  NULL){
		gLog->Write("[%s][%s][%d]Check the Pointer is NULL", __FILE__, __FUNCTION__, __LINE__);
		return (const char *)NULL;
	}
	else
	{
		if( (l_delimiters == NULL) || (strlen(l_delimiters)==0) ){
			return (const char *)NULL;
		}
		
		size_t chbuf_len = strlen(chbuf);
		size_t dm_len    = strlen(l_delimiters);
		size_t ch_pos, i, j;
		bool   bCheck  = true;
		
		for(ch_pos=0, i=0; i<chbuf_len; i++){
			
			bCheck=true;
			for(j=0; j<dm_len; j++){
				if(chbuf[i]==l_delimiters[j]){ // charactor is same
					bCheck=false;
					break;
				}
				else{
					//do nothing
				}
			}

			if(bCheck){
				chbuf[ch_pos++]=chbuf[i];
			}
		}

		chbuf[ch_pos]='\0';
		return chbuf;
	}
		
};


//-----------------------------------------------------
// ���Ϸκ��� ������ ����Ʈ�� ����(�����۾�)
//-----------------------------------------------------
int CEmsMXManager::getFromDomainList(const char * dname, std::vector<string> &vecDomainList)
{
	char  tmpFileBuf[NS_MAXDNAME+1] = {0,};
	FILE *fp = fopen(dname, "r");
	
	if(fp == NULL){
		//Print Log
		gLog->Write("[%s][%s][%d] File Open failed![%s]", __FILE__, __FUNCTION__, __LINE__, dname);
		return -1;
	}
	
	while(!feof(fp)){
		memset((void *)&tmpFileBuf, 0, NS_MAXDNAME+1);
		fgets(tmpFileBuf, NS_MAXDNAME, fp);
		if(strlen(tmpFileBuf) == 0){
			continue;
		}

		const char *pBuf = trim_chars(tmpFileBuf, DELIMITERS);
		if(strlen(pBuf) > 0 )
			vecDomainList.push_back(pBuf);
	}
	fclose(fp);	

	return 0;
};

//-----------------------------------------------------
// ���������κ��� MX ����Ʈ ���� ����
//-----------------------------------------------------
bool CEmsMXManager::InsertMX(const char *dname, int &errCode, string &errStr)
{
	if(dname ==  NULL){
		errCode = -1; // Domain Name  is NULL
		errStr  = "(Internal) Domain Name is Null";
		gLog->Write("[%s][%s][%d][ERROR][Domain Name is NULL][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, errCode);
		return false;
	}
	else{

		mapMXList::iterator itr;
		mapMXList * pMXList = this->getMXList();

		if(pMXList == (mapMXList *)NULL){ // Exit ó���� �Ͽ� ���μ��� ������� �ϵ��� �Ѵ�.		
			                                //MXList map�� �������� �ʾ����Ƿ� �ý��������� ������ �ְų� 
			                                //���μ��� ���������� �ʴ� ���¶�� �Ǵܵ�
			errCode = -2;                   //mxpMXList ���� �������� <= ischoi �α׶� ������ ���� ���Ǹ� �غ���
			errStr  = "(Internal) MXList is NULL"; 
			gLog->Write("[%s][%s][%d][ERROR][getMXList() Failed][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, errCode);
			return false;
		}
			
		itr = pMXList->find(dname);
		if(itr != pMXList->end()){ // DomainName �� �����ϹǷ� ���� ���� ����
			pMXList->erase(itr);
		}
		
		// CEmsMX ���� ���� �� ����
		shared_ptr<CEmsMX> tmpEmsMX = shared_ptr<CEmsMX>(new CEmsMX(dname));
		CEmsMX * pEmsMx = tmpEmsMX.get();
		if(pEmsMx != NULL){
			if(pEmsMx->MXLookup(dname) < 0) {  //<== ischoi �ӽ÷� -1��(������) ��� �������� �ʵ��� �� 
				errCode = -3;  // MXLookup ����
				errStr  = "CEmsMX Create Failed(E)";
				//errStr.append(dname);
				gLog->Write("[%s][%s][%d][ERROR][%s][MXLookup Failed][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, dname, errCode);
				return false; 
			}
			
			pMXList->insert(std::make_pair<string, shared_ptr<CEmsMX> >(string(dname), tmpEmsMX) );
		}
		else {
			//Print Log :  CEmsMX ���� ����
			errCode = -4;
			errStr  = "CEmsMX Create Failed(I)";
			gLog->Write("[%s][%s][%d][ERROR][%s][CEmsMX Create Failed][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, dname, errCode);
			return false;
		}
		return true;	
	}

};
	

shared_ptr<CEmsMX> CEmsMXManager::getMXFromMXList(const char *dname, int &errCode, string &errStr)
{
	shared_ptr<CEmsMX> retspMX;
	mapMXList * theMXList = this->getMXList();
	const char *pdname = theDomainList()->checkDuplicateDNS(dname);
	

	if(theMXList == (mapMXList *)NULL){ // Exit ó���� �Ͽ� ���μ��� ������� �ϵ��� �Ѵ�.		
		              //MXList map�� �������� �ʾ����Ƿ� �ý��������� ������ �ְų� ���μ��� ���������� �ʴ� ���¶�� �Ǵܵ�
		errCode = -4; //mxpMXList ���� ��������
		//Print Log
		gLog->Write("[%s][%s][%d][ERROR][%s] mxpMXList Create Failed [ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, pdname, errCode);
		return retspMX;
	}
	else {
		mapMXList::iterator itr;
			
		//MX List Check
		if(theMXList->size() == 0 ) {
			//dname is not exist
		}
		else{
			itr = theMXList->find(pdname);
			if(itr != theMXList->end()){
				retspMX = ( shared_ptr<CEmsMX> )(itr->second);
				return retspMX;
			}
		}

		if(InsertMX (pdname, errCode, errStr) == true){
			itr = theMXList->find(pdname);
			if(itr != theMXList->end()){
				retspMX = ( shared_ptr<CEmsMX> )(itr->second);
			}
		}

		return retspMX;
	}
};
	

mapMXList * CEmsMXManager::getMXList(){ // MXList Return
	if(m_pMapMXList == NULL){
			m_pMapMXList = new mapMXList;
	}
	
	return m_pMapMXList;
}; 


bool CEmsMXManager::checkUsable(const char *pDName, int interval_msec)
{
	//check time difference
	shared_ptr<CEmsMX> retspMX;
	mapMXList * theMXList = this->getMXList();

	if(theMXList == (mapMXList *)NULL){ // Exit ó���� �Ͽ� ���μ��� ������� �ϵ��� �Ѵ�.		
		gLog->Write("[%s][%s][%d][ERROR][%s] mxpMXList Create Failed", __FILE__, __FUNCTION__, __LINE__, pDName);
		return false;
	}
	else {
		mapMXList::iterator itr;
		//MX List Check
		itr = theMXList->find(pDName);
		if(itr != theMXList->end()){
			retspMX = ( shared_ptr<CEmsMX> )(itr->second);
		}
		else{
			int    errCode;
			string errStr;
			if(InsertMX (pDName, errCode, errStr) == true){
				itr = theMXList->find(pDName);
				if(itr != theMXList->end()){
					retspMX = ( shared_ptr<CEmsMX> )(itr->second);
				}
			}
			else{
				gLog->Write("[%s][%s][%d][ERROR][%s] MX Create Failed [ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, pDName, errCode);
				return false;
			}
		}

		if(retspMX.get() != NULL){
			vecMXList          *pMXList = retspMX.get()->getMXList();
			vecMXList::iterator itr_mx;
				
			if(pMXList != NULL){
				boost::shared_ptr<_MXRecord> pmxRecord;
				int iTimeDiff;
				struct timeval CurrTime;
				gettimeofday(&CurrTime, NULL);

				for(itr_mx = pMXList->begin();itr_mx != pMXList->end(); itr_mx++){
					//gLog->Write("[IPCHECK][IP:%s][_iConnCnt :%d][_ns_retrycount:%d]", itr_mx->get()->_ns_addr, itr_mx->get()->_iConnCnt, itr_mx->get()->_ns_retrycount);
					//if((itr_mx->get()->_ns_retrycount < m_iMaxRetryCount) && (itr_mx->get()->_iConnCnt < 1)){
					if(itr_mx->get()->_iConnCnt < 1){
						iTimeDiff = ( CurrTime.tv_sec - itr_mx->get()->_lastConnTime.tv_sec ) * 1000 +
						            ( CurrTime.tv_usec - itr_mx->get()->_lastConnTime.tv_usec ) / 1000;

						if(iTimeDiff > interval_msec){
							return true;
						}
					}
				}
			}
			else{
				gLog->Write("[%s][%s][%d][ERROR][%s] MX List IS NULL", __FILE__, __FUNCTION__, __LINE__, pDName);
			}
		}
		else{
			gLog->Write("[%s][%s][%d][ERROR][%s] MX IS NULL", __FILE__, __FUNCTION__, __LINE__, pDName);
		}
		return false;
	}
}

void CEmsMXManager::closeDomainConnIP(const char *pDName, const char *pIPAddr)
{
		//check time difference
	shared_ptr<CEmsMX> retspMX;
	mapMXList * theMXList = this->getMXList();

	if(theMXList == (mapMXList *)NULL){ // Exit ó���� �Ͽ� ���μ��� ������� �ϵ��� �Ѵ�.		
		gLog->Write("[%s][%s][%d][ERROR][%s] mxpMXList Create Failed", __FILE__, __FUNCTION__, __LINE__, pDName);
	}
	else {
		mapMXList::iterator itr;
		itr = theMXList->find(pDName);

		if(itr != theMXList->end()){
			retspMX = itr->second;
			retspMX.get()->decConnCount(pIPAddr );
		}
	}
}

