
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
	//m_CheckThread Thread 종료시키기
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
// MxManager Thread에서 MX 리스트를 주기적으로 갱신처리
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

		//1.갱신 시간이 지난 경우
		if( tmpEmsMX.get()->compareTime(tmpEmsMX.get()->getMXBuildTime(), iRefreshTime) == true ){
			//gLog->Write("[2][XM][ERASE][%s][iRefreshTime:%d]", itr_map->first.c_str(), iRefreshTime); 
			itr_map = pMXList->erase(itr_map);
			bRefresh = true;
			continue;
		}
		//2.Retry Count 많을 경우
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
// 쓰레드 실행함수
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
// MX List의 데이터를 전부 삭제하도록 함
//-----------------------------------------------
bool CEmsMXManager::DeleteMXAll(){
	if( (m_pMapMXList != NULL) && (m_pMapMXList->size() > 0)){
		m_pMapMXList->clear();
	}
	
	return true;
}

//-----------------------------------------------
// Trim 같은 경우는 공용 Function으로 분리하여 사용
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
// 파일로부터 도메인 리스트를 생성(선행작업)
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
// 도메인으로부터 MX 리스트 생성 삽입
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

		if(pMXList == (mapMXList *)NULL){ // Exit 처리를 하여 프로세스 재시작을 하도록 한다.		
			                                //MXList map이 생성되지 않았으므로 시스템적으로 문제가 있거나 
			                                //프로세스 정상동작하지 않는 상태라고 판단됨
			errCode = -2;                   //mxpMXList 생성 에러리턴 <= ischoi 로그랑 에러에 대한 정의를 해보자
			errStr  = "(Internal) MXList is NULL"; 
			gLog->Write("[%s][%s][%d][ERROR][getMXList() Failed][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, errCode);
			return false;
		}
			
		itr = pMXList->find(dname);
		if(itr != pMXList->end()){ // DomainName 이 존재하므로 기존 정보 삭제
			pMXList->erase(itr);
		}
		
		// CEmsMX 정보 생성 및 삽입
		shared_ptr<CEmsMX> tmpEmsMX = shared_ptr<CEmsMX>(new CEmsMX(dname));
		CEmsMX * pEmsMx = tmpEmsMX.get();
		if(pEmsMx != NULL){
			if(pEmsMx->MXLookup(dname) < 0) {  //<== ischoi 임시로 -1인(에러인) 경우 삽입하지 않도록 함 
				errCode = -3;  // MXLookup 실패
				errStr  = "CEmsMX Create Failed(E)";
				//errStr.append(dname);
				gLog->Write("[%s][%s][%d][ERROR][%s][MXLookup Failed][ECODE:%d]", __FILE__, __FUNCTION__, __LINE__, dname, errCode);
				return false; 
			}
			
			pMXList->insert(std::make_pair<string, shared_ptr<CEmsMX> >(string(dname), tmpEmsMX) );
		}
		else {
			//Print Log :  CEmsMX 생성 에러
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
	

	if(theMXList == (mapMXList *)NULL){ // Exit 처리를 하여 프로세스 재시작을 하도록 한다.		
		              //MXList map이 생성되지 않았으므로 시스템적으로 문제가 있거나 프로세스 정상동작하지 않는 상태라고 판단됨
		errCode = -4; //mxpMXList 생성 에러리턴
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

	if(theMXList == (mapMXList *)NULL){ // Exit 처리를 하여 프로세스 재시작을 하도록 한다.		
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

	if(theMXList == (mapMXList *)NULL){ // Exit 처리를 하여 프로세스 재시작을 하도록 한다.		
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

