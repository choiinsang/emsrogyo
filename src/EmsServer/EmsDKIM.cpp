//********************************************************************
//*- 최초 파일을 이용하여 Signature 생성
//*- DNS 사용하여 Signature를 생성하는 부분은 차후 고려하도록 한다.
//*- by ischoi
//********************************************************************/
#include <stdio.h>
#include <string.h>
#include "EmsDKIM.h"
#include "EmsLog.h"

extern CEmsLog * gLog;

#ifndef MIN
 #define MIN(x,y)	((x) < (y) ? (x) : (y))
#endif /* ! MIN */

//---------------------------------------------------
//class CEmsDKIMLib
//---------------------------------------------------
CEmsDKIMLib::CEmsDKIMLib()
{
}

CEmsDKIMLib::~CEmsDKIMLib()
{
	if(m_pstDKIMLib != NULL){
		dkim_close(m_pstDKIMLib);
		m_pstDKIMLib = NULL;
	}
}
		
DKIM_LIB * CEmsDKIMLib::getInstance()
{
	if(m_pstDKIMLib == NULL){
		/* instantiate the library */
		m_pstDKIMLib = dkim_init(NULL, NULL); 
		if(m_pstDKIMLib == NULL){
			fprintf(stderr, "[%s][%s][%d] libopendkim Open Error!", __FILE__, __FUNCTION__, __LINE__);
			return NULL;
		}
	}
	return m_pstDKIMLib;
}
	
DKIM_LIB *CEmsDKIMLib::m_pstDKIMLib = NULL;


//---------------------------------------------------
//class CEmsDKIM : public LockObject
//---------------------------------------------------
CEmsDKIM::CEmsDKIM (const char *keypath
                  , const char *selector
                  , const char *domainName
                  , const char *pHeaderCanon
                  , const char *pBodyCanon
                  , dkim_alg_t signalg)
{
	m_pSigKey  = NULL;
	
	this->Lock();
	m_pDKIMLib = theDKIMLib();
	this->Unlock();

	m_pSelector   = strdup(selector);	
	m_pDomainName = strdup(domainName);
	
	if(keypath != NULL)
		m_pSigKeyPath = strdup(keypath);
	else
		m_pSigKeyPath = NULL;
	
	m_hdrCanon    = getCanonCode(pHeaderCanon);
	m_bodyCanon   = getCanonCode(pBodyCanon);
	
	m_signAlg     = signalg;	
	
	checkSignAlg(m_signAlg);
}

CEmsDKIM::~CEmsDKIM()
{

	if(m_pSelector != NULL)
		free(m_pSelector);

	if(m_pDomainName != NULL)
		free(m_pDomainName);

	if(m_pSigKeyPath != NULL)
		free(m_pSigKeyPath);
	
}

		
bool CEmsDKIM::setKeyPath(const char *keypath)
{
	if(m_pSigKeyPath != NULL){
		free(m_pSigKeyPath);
		m_pSigKeyPath = NULL;
	}
	
	if(keypath != NULL)
		m_pSigKeyPath = strdup(keypath);
	else
		m_pSigKeyPath = NULL;
		
	if(m_pSigKeyPath != NULL)
		return true;
	else 
		return false;
}


dkim_canon_t CEmsDKIM::getCanonCode(const char *strCanon)
{ //if received character is Unknown, return simple canonicalization.
	if (strCanon == NULL){
		return (dkim_canon_t) DKIM_CANON_SIMPLE;
	}
	else if (strcasecmp(strCanon, "simple") == 0)
		return (dkim_canon_t) DKIM_CANON_SIMPLE;
	else if (strcasecmp(strCanon, "relaxed") == 0)
		return (dkim_canon_t) DKIM_CANON_RELAXED;
	else{
		return (dkim_canon_t) DKIM_CANON_SIMPLE;
	}
}


const char* CEmsDKIM::getCanonName(dkim_canon_t canonCode)
{
	switch (canonCode)
	{
	  case DKIM_CANON_SIMPLE:
		return "simple";

	  case DKIM_CANON_RELAXED:
		return "relaxed";

	  case DKIM_CANON_UNKNOWN:
	  default:
		return "unknown";
	}
}


dkim_alg_t CEmsDKIM::getAlgCode(const char *pAlgName)
{
	if(pAlgName == NULL)
		return (dkim_alg_t) DKIM_SIGN_UNKNOWN;
	else if (strcasecmp(pAlgName, "rsa-sha1") == 0)
		return (dkim_alg_t) DKIM_SIGN_RSASHA1;
	else if (strcasecmp(pAlgName, "rsa-sha256") == 0)
		return (dkim_alg_t) DKIM_SIGN_RSASHA256;
	else
		return (dkim_alg_t) DKIM_SIGN_UNKNOWN;
}


//-------------------------------------------------
//--ischoi  DKIM  설정 정보를 재설정 하기 위한 목적
bool CEmsDKIM::setDKIMInfo (const char   *keypath
				                  , const char   *selector
				                  , const char   *domainName
				                  , dkim_canon_t headerCanon
				                  , dkim_canon_t bodyCanon
				                  , dkim_alg_t   signalg)
{
	this->Lock();
	m_pDKIMLib = theDKIMLib();
	this->Unlock();

	if(m_pSelector != NULL) 
		free(m_pSelector);
	m_pSelector = strdup(selector);	
	
	if( m_pDomainName != NULL)
		free(m_pDomainName);
	m_pDomainName = strdup(domainName);	
	
	if(m_pSigKeyPath != NULL)
		free(m_pSigKeyPath);
	if(keypath != NULL)
		m_pSigKeyPath = strdup(keypath);
	else
		m_pSigKeyPath = NULL;
	
	if(m_pSigKey != NULL){
		free(m_pSigKey);
		m_pSigKey = NULL;
	}
	m_pSigKey = (dkim_sigkey_t)getSignKey(m_pSigKeyPath);
	if(m_pSigKey == NULL)
		return false;
	
	m_hdrCanon  = headerCanon;
	m_bodyCanon = bodyCanon;
	m_signAlg   = signalg;
	
	return true;
}

//--------------------------------------------------
//--ishcoi: Return DKIM pointer: 개별 객체생성 전달
//--------------------------------------------------
DKIM * CEmsDKIM::createDKIM_sign(const char *jobID, DKIM_STAT &status)
{
	DKIM * pdkim = NULL;
	try{
		this->Lock();
		{
			if(m_pDKIMLib == NULL){
				m_pDKIMLib = theDKIMLib();
	
				if(m_pDKIMLib == NULL){
					this->Unlock();
					throw string("DKIM Library Open Failed.");
				}
			}

			u_int flags;
			status = dkim_options(m_pDKIMLib, DKIM_OP_GETOPT, DKIM_OPTS_FLAGS, &flags, sizeof(flags) );
			if(status != DKIM_STAT_OK ){
				this->Unlock();
				throw string("DKIM Library Get Option Failed.");
			}

			flags |= DKIM_LIBFLAGS_SIGNLEN;
			status = dkim_options(m_pDKIMLib, DKIM_OP_SETOPT, DKIM_OPTS_FLAGS, &flags,
	                    sizeof flags);
			if(status != DKIM_STAT_OK ){
				this->Unlock();
				throw string("DKIM Library Set Option Failed.");
			}
			
			if(m_pSigKey != NULL){
				free(m_pSigKey);
				m_pSigKey = NULL;
			}
			
			m_pSigKey = (dkim_sigkey_t)getSignKey(m_pSigKeyPath);

			if(m_pSigKey == NULL){
				this->Unlock();
				return (DKIM *)NULL;
			}
				
			pdkim = dkim_sign(m_pDKIMLib
			                , (const unsigned char*)jobID
			                , NULL
			                , m_pSigKey
			                , (const unsigned char*)m_pSelector
			                , (const unsigned char*)m_pDomainName
			                , m_hdrCanon
			                , m_bodyCanon
			                , m_signAlg
			                , -1L
			                , &status);
		}
		this->Unlock();
	}
	catch(string err){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, err.c_str());
		return (DKIM *)NULL;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, "UNKNOWN Error Occourred.");
		return (DKIM *)NULL;
	}
	
	return pdkim;
}


//--------------------------------------------
// Signature Key Path 설정 및 DKIM* 가져오기
//--------------------------------------------
DKIM * CEmsDKIM::createDKIM_verify(const char* jobID, DKIM_STAT &status)
{	
	DKIM * pdkim = NULL;
	try{
		this->Lock();
		{
			if(m_pDKIMLib == NULL){
				m_pDKIMLib = theDKIMLib();
	
				if(m_pDKIMLib == NULL){
					this->Unlock();
					throw string("DKIM Library Open Failed.") + string(__FUNCTION__);
				}
			}
		}
		this->Unlock();
			
		pdkim = dkim_verify(m_pDKIMLib, (const unsigned char*)jobID, NULL, &status);

		if(pdkim == NULL)
			return (DKIM*)NULL;
				
		return pdkim;			
	}
	catch(string err){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, err.c_str());
		return (DKIM *)NULL;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, "UNKNOWN Error Occourred.");
		return (DKIM *)NULL;
	}

	return (DKIM*)NULL;
}

//--------------------------------------------
// Signature Key Path 설정
//--------------------------------------------
bool CEmsDKIM::setDKIMLibOpt_verify(const char* keyFilePath, const char* jobID, DKIM_STAT &status)
{
	bool retval = false;
	
	this->Lock();
	try{
		while(1){
			if(m_pDKIMLib == NULL){
				m_pDKIMLib = theDKIMLib();
	
				if(m_pDKIMLib == NULL){
					throw string("DKIM Library Open Failed.") + string(__FUNCTION__);
				}
			}

			u_int flags;
			status = dkim_options(m_pDKIMLib, DKIM_OP_GETOPT, DKIM_OPTS_FLAGS, &flags, sizeof(flags) );
			if(status != DKIM_STAT_OK ){
				break;
			}

			flags |= DKIM_LIBFLAGS_SIGNLEN;
			status = dkim_options(m_pDKIMLib, DKIM_OP_SETOPT, DKIM_OPTS_FLAGS, &flags,
	                    sizeof flags);
			if(status != DKIM_STAT_OK ){
				break;
			}

			dkim_query_t qtype = DKIM_QUERY_FILE;
			status = dkim_options(m_pDKIMLib, DKIM_OP_SETOPT, DKIM_OPTS_QUERYMETHOD, &qtype, sizeof(qtype) );
			if(status != DKIM_STAT_OK ){
				break;
			}
		
			status = dkim_options(m_pDKIMLib, DKIM_OP_SETOPT, DKIM_OPTS_QUERYINFO,
			                    (void *)keyFilePath, strlen(keyFilePath) );
			if(status != DKIM_STAT_OK ){
				break;
			}
			
			retval = true;		
			break;
		}
	}
	catch(string err){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, err.c_str());
		retval = false;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR] %s", __FUNCTION__, __FILE__, __LINE__, "UNKNOWN Error Occourred.");
		retval = false;
	}
	this->Unlock();
	
	return retval;
}



//--------------------------------------------
// Signature Algorithm이 지원하는 형식인지 체크
//--------------------------------------------
bool CEmsDKIM::checkSignAlg(dkim_alg_t &signalg)
{
	if (signalg == DKIM_SIGN_UNKNOWN)	{
		if (dkim_libfeature(m_pDKIMLib, DKIM_FEATURE_SHA256))
			signalg = DKIM_SIGN_RSASHA256;
		else
			signalg = DKIM_SIGN_RSASHA1;
	}
	else if ( (signalg == DKIM_SIGN_RSASHA256) &&
	         !dkim_libfeature(m_pDKIMLib, DKIM_FEATURE_SHA256)) {
		gLog->Write("[%s][%s][%d][DKIM_FEATURE_SHA256] requested signing algorithm not available", __FILE__, __FUNCTION__, __LINE__);
		dkim_close(m_pDKIMLib);
		return false;
	}
	else if ( (signalg == DKIM_SIGN_RSASHA1) &&
	         !dkim_libfeature(m_pDKIMLib, DKIM_FEATURE_SHA256)) {
		gLog->Write("[%s][%s][%d][DKIM_FEATURE_SHA256] requested signing algorithm not available", __FILE__, __FUNCTION__, __LINE__);
		dkim_close(m_pDKIMLib);
		return false;
	}

	return true;
}

//--------------------------------------------
// Message Header 추가
//--------------------------------------------
DKIM_STAT CEmsDKIM::setDKIMHeader(DKIM *pdkim, const char *phBuf) 
{
	try{
		if((pdkim != NULL) && ( phBuf != NULL)){
			return dkim_header(pdkim, (u_char*)phBuf, strlen(phBuf));
		}
		else 
			return DKIM_STAT_INVALID;
	}
	catch(...){
		return DKIM_STAT_INTERNAL;
	}		
}

//--------------------------------------------
// Signature Message Header [End of Header]
//--------------------------------------------
DKIM_STAT CEmsDKIM::setDKIMEOH(DKIM *pdkim) 
{
	if(pdkim != NULL){
		return dkim_eoh(pdkim);
	}
	return DKIM_STAT_INTERNAL;
}

//--------------------------------------------
// Message Header 추가
//--------------------------------------------
DKIM_STAT CEmsDKIM::setDKIMBody(DKIM *pdkim, const char *phBuf) 
{
	try{
		if((pdkim != NULL) && ( phBuf != NULL)){
			char     *pbodybuf = const_cast<char*>(phBuf);
			int       bodylen  = strlen(phBuf);
			DKIM_STAT status   = DKIM_STAT_OK;

			if(bodylen > DEFAULT_BODY_LEN){
				int wsz = 0;
				while (bodylen > 0)
				{
					wsz    = MIN(DEFAULT_BODY_LEN, bodylen);	
					status = dkim_body(pdkim, (u_char*)pbodybuf, wsz);
					if(status != DKIM_STAT_OK)
						break;
						
					pbodybuf += wsz;
					bodylen  -= wsz;
				}
			}
			else{
				status = dkim_body(pdkim, (u_char*)pbodybuf, bodylen);	
			}
			
			return status;
		}
		else 
			return DKIM_STAT_INVALID;
	}
	catch(...){
		return DKIM_STAT_INTERNAL;
	}		
}

//--------------------------------------------
// Signature Message Body [End Of Message]
//--------------------------------------------
DKIM_STAT CEmsDKIM::setDKIMEOM(DKIM *pdkim)
{
	if(pdkim != NULL){
		return dkim_eom(pdkim, NULL);
	}	
	return DKIM_STAT_INTERNAL;
}

//--------------------------------------------
// Get Signatured Header Message
//--------------------------------------------
DKIM_STAT CEmsDKIM::getSignedHeader(DKIM *pdkim, char * phdr, size_t phdrlen)
{
	if(pdkim != NULL){
		int dkimSigHeaderLen = 0; 
		sprintf(phdr, "%s: ", DKIM_SIGNHEADER);
		dkimSigHeaderLen = strlen(phdr);
		return dkim_getsighdr(pdkim, (u_char*)(phdr + dkimSigHeaderLen),
		                        phdrlen - dkimSigHeaderLen,
		                        dkimSigHeaderLen + 1);
	}
	else
		return DKIM_STAT_INTERNAL;
}

//--------------------------------------------
// DKIM Close
//--------------------------------------------
DKIM_STAT CEmsDKIM::closeDKIM(DKIM *pdkim)
{
	return dkim_free(pdkim);
}

//--------------------------------------------
// Signature Key Load
//--------------------------------------------
char * CEmsDKIM::getSignKey(const char *pKeyFilePath)
{
	if(pKeyFilePath == NULL)
		return (char*)NULL;
	else{
		char   tmpkeybuffer[SZ_DEFAULTBUFFER+1] = {0,};
		FILE * fp_key = NULL;
		
		fp_key = fopen(pKeyFilePath, "r");
		if(fp_key == NULL){
			gLog->Write("[File Open failed][PATH:%s]", __FILE__, __FUNCTION__, __LINE__, pKeyFilePath);
			return (char*)NULL;
		}
		
		fread(tmpkeybuffer, SZ_DEFAULTBUFFER, 1, fp_key);
		fclose(fp_key);
		
		return (char*)strdup(tmpkeybuffer);
	}
}

