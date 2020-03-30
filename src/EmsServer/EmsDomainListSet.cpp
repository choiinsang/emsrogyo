//********************************************************************
//*- Domain 별로 연결 제한 혹은 연결 시간 제한을 두어 접속
//*- by ischoi
//********************************************************************/
#include <stdio.h>
#include <string.h>
#include "EmsDomainListSet.h"
#include "EmsLog.h"
#include "EmsMXManager.h"

extern CEmsLog * gLog;

//---------------------------------------------------
//class CEmsDomainListSet
// Works: 정의된 도메인에 대해서 연결 개수와 delaytime을 정의한다.
//---------------------------------------------------
CEmsDomainListSet::CEmsDomainListSet()
{
	m_spMapDomainListInfo = shared_ptr< unordered_map<string, strDomainInfo> >(new unordered_map<string, strDomainInfo> ) ;
}

CEmsDomainListSet::~CEmsDomainListSet()
{
}

CEmsDomainListSet *CEmsDomainListSet::m_pEmsDomainListSet = NULL;

CEmsDomainListSet *CEmsDomainListSet::getInstance()
{
	if(m_pEmsDomainListSet == NULL){
		m_pEmsDomainListSet =  new CEmsDomainListSet();
	}
	
	return m_pEmsDomainListSet;
}

//---------------------------------------------------
//Set DomainList Info
//---------------------------------------------------
bool CEmsDomainListSet::setDomainListInfo (const char *fpath)
{
	char delimiter1 = '|';
	char delimiter2 = ':';
	//Parsing
	if(fpath != NULL){
		//파일의 도메인 리스트를 라인별로 파싱한다.
		char  tmpFileBuf[SZ_NAME+1] = {'\0',};
		FILE *fp = fopen(fpath, "r");
		if(fp == NULL){
			//Print Log
			gLog->Write("[%s][%s][%d] File Open failed![%s]", __FILE__, __FUNCTION__, __LINE__, fpath);
			return false;
		}
		Lock();
		while(!feof(fp)){
			memset((void *)&tmpFileBuf, 0, SZ_NAME+1);
			fgets(tmpFileBuf, SZ_NAME, fp);
			if(strlen(tmpFileBuf) == 0){
				continue;
			}
	
			//Trim Space
			size_t ipos=0;
			for(int i=0; i<strlen(tmpFileBuf); i++){
				if((tmpFileBuf[i]==' ')||(tmpFileBuf[i]=='\t')||(tmpFileBuf[i]=='\n')){
					continue;
				}
				else{
					tmpFileBuf[ipos]=tmpFileBuf[i];
					ipos++;
				}
			}
			tmpFileBuf[ipos]='\0';
			
			if(ipos < 2){
				tmpFileBuf[ipos]='\0';
				continue;
			}
			else{  //Line Parsing

				vector<string> vecDomainName;
				strDomainInfo  tmpDomainInfo;
				bool           bDomainNameExist = false;
				
				bool   bRet   = true;
				size_t buflen = strlen(tmpFileBuf);
				       ipos   = 0;

				while(ipos < buflen){
					size_t i=ipos;
					char tmpBuf[SZ_NAME+1] = {'\0',};
					for(; i<buflen; i++){
						if( (tmpFileBuf[i]==delimiter1)
							||(tmpFileBuf[i]=='\0')
							||(tmpFileBuf[i]=='\n')){
								break;  //for()
						}
					}
					
					if(i==ipos){
						ipos++;
						continue;
					}
					else{
						memcpy(tmpBuf, &tmpFileBuf[ipos], i-ipos);
						ipos = ++i;
					}

					//(Type)[1byte](Delimiter(:))[1byte]
					if(strlen(tmpBuf) < 3)
						continue;   //while()
					if(tmpBuf[1]!=delimiter2)
						continue;
						
					switch(tmpBuf[0]){
						case 'D':{  //Domain
							vecDomainName.push_back(string(&tmpBuf[2]));
							bDomainNameExist = true;
							break;
						}
						case 'T':{  //Delay Time
							tmpDomainInfo.setDelay(atoi(&tmpBuf[2]));
							break;
						}
						case 'C':{  //Max Client Count
							tmpDomainInfo.setMaxCon(atoi(&tmpBuf[2]));
							break;
						}
						default:
							bRet=false;
					}
				}

				if(bDomainNameExist==true){
					timeval *pLastTime = tmpDomainInfo.getLastTime();
					gettimeofday(pLastTime, NULL);
					m_spMapDomainListInfo.get()->insert(std::pair<string, strDomainInfo>(string(vecDomainName.at(0)), tmpDomainInfo));
					
					for(vector<string>::iterator itr_dns = vecDomainName.begin(); itr_dns != vecDomainName.end(); itr_dns++){
						m_mapDuplicateDNS.insert(std::pair<string, string>(*itr_dns, *vecDomainName.begin()));
					}
				}
			}
		}
		Unlock();
		fclose(fp);
	}
	return true;
}

//---------------------------------------------------
// 중복 DNS 체크
//---------------------------------------------------
const char * CEmsDomainListSet::checkDuplicateDNS(const char *pDnsName)
{
	const char * pDNSName;
	unordered_map<string, string>::iterator itr_dp = m_mapDuplicateDNS.find(string(pDnsName));
	if(itr_dp != m_mapDuplicateDNS.end())
		pDNSName = itr_dp->second.c_str();
	else
		pDNSName = pDnsName;
	
	return pDNSName;
}


//---------------------------------------------------
//Get Mail Send Interval
//---------------------------------------------------
bool CEmsDomainListSet::getDomainInterval(const char *pDomainName, int &interval, int &conCnt)
{
	bool retval = true;

	if(pDomainName == NULL){
		gLog->Write("[%s][%s][%d][ERROR][Domain Name is NULL]", __FILE__, __FUNCTION__, __LINE__);
		retval = false;
	}
	else{
		const char *pTmpDomainName = checkDuplicateDNS(pDomainName);
		unordered_map<string, strDomainInfo>::iterator itr_map = m_spMapDomainListInfo.get()->find(string(pTmpDomainName));
		if(itr_map != m_spMapDomainListInfo.get()->end()){
			try{
				strDomainInfo *pDomainInfo = (strDomainInfo*)&itr_map->second;
				interval                   = pDomainInfo->getDelay();
				conCnt                     = pDomainInfo->getMaxCon();
			}
			catch(...){
				gLog->Write("[%s][%s][%d]Unknown Error Occurred", __FILE__, __FUNCTION__, __LINE__);	
			}
		}
		else{
			interval = DEFAULT_INTERVAL_MSEC;
			conCnt   = 1;
		}
	}
	return retval;
}


//---------------------------------------------------
//Check Domain Connect Usable or Not
//---------------------------------------------------
bool CEmsDomainListSet::isConnectDomainUsable(const char *pDomainName, int &interval)
{
	if(pDomainName == NULL){
		gLog->Write("[%s][%s][%d][ERROR][Domain Name is NULL]", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}
	else{
		bool        bRet=true;
		const char *pTmpDomainName = checkDuplicateDNS(pDomainName);
		unordered_map<string, strDomainInfo>::iterator itr_map = m_spMapDomainListInfo.get()->find(string(pTmpDomainName));
		
		if(itr_map != m_spMapDomainListInfo.get()->end()){
			Lock();
			try{
				strDomainInfo *pDomainInfo    = (strDomainInfo*)&itr_map->second;
				interval                      = pDomainInfo->getDelay();
				bRet = theMXManager()->checkUsable(pTmpDomainName, interval);
			}
			catch(...){
				gLog->Write("[%s][%s][%d]Unknown Error Occurred", __FILE__, __FUNCTION__, __LINE__);	
			}
			Unlock();
		}
		else{
			interval = DEFAULT_INTERVAL_MSEC;
		}
		
		return bRet;
	}
}


//---------------------------------------------------
// 연결 종료/세션 관리
//---------------------------------------------------
void CEmsDomainListSet::disconnClient(const char *pDomainName, const char *pIPAddr)
{
	const char    *pTmpDomainName = checkDuplicateDNS(pDomainName);
	unordered_map<string, strDomainInfo>::iterator itr_map = m_spMapDomainListInfo.get()->find(string(pTmpDomainName));
	//DNS 리스트에 있는 경우
	if(itr_map != m_spMapDomainListInfo.get()->end()){
		Lock();
		//strDomainInfo *pDomainInfo = (strDomainInfo*)&itr_map->second;
		//pDomainInfo->decrConnCount();
		theMXManager()->closeDomainConnIP(pTmpDomainName, pIPAddr);
		Unlock();
	}
	//DNS 리스트에 없는 경우
	else{
		Lock();
		theMXManager()->closeDomainConnIP(pDomainName, pIPAddr);
		Unlock();
	}
}


void CEmsDomainListSet::printListInfo()
{
	unordered_map<string, strDomainInfo>::iterator itr_map = m_spMapDomainListInfo.get()->begin();
	for(;itr_map != m_spMapDomainListInfo.get()->end(); itr_map++){
		strDomainInfo tmpDomainInfo = itr_map->second;
		//gLog->Write("[Domain:%s][DelayTime:%d][Client:%d]", itr_map->first.c_str(), tmpDomainInfo.getDelay(), tmpDomainInfo.getMaxCon());
	}
}



