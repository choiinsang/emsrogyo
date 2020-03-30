//**************************************
//* DKIM Signature Process Implamentation
//* Using open source 'OpenDKIM'.
//* Need libopendkim.so Libaray. 
//* by ischoi
//* date : 2015. 06. x
//**************************************/
#ifndef __CEMS_DKIM_HEADER__
#define __CEMS_DKIM_HEADER__

#include <dkim.h>

#include "LockObject.h"
#include "DefineNetGlobal.h"
#include "EmsCommon.h"

using namespace boost;
using namespace std;

class CEmsDKIMLib
{
	public:
		~CEmsDKIMLib();
		
		static DKIM_LIB *getInstance();

	private:
		CEmsDKIMLib();
	
	private:
		static DKIM_LIB *m_pstDKIMLib;
};


//--ischoi Ems Message를 DKIM 객체
class CEmsDKIM : public LockObject
{
	public:

		CEmsDKIM (const char *keypath   , const char *selector
		        , const char *domainName, const char *headerCanon
		        , const char *bodyCanon , dkim_alg_t signalg);
		~CEmsDKIM();
		
		
		//DKIM_LIB *   getDKIMLib  ();
		bool      setKeyPath  (const char   *keypath);
		bool      setDKIMInfo (const char   *keypath,    const char   *selector
		                     , const char   *domainName, dkim_canon_t  headerCanon
		                     , dkim_canon_t  bodyCanon,  dkim_alg_t    signalg);
						                  
		DKIM     *createDKIM_sign     (const char *jobID, DKIM_STAT &status);
		DKIM     *createDKIM_verify   (const char* jobID, DKIM_STAT &status);
		bool      setDKIMLibOpt_verify(const char* keyFilePath, const char* jobID, DKIM_STAT &status);

		bool          checkSignAlg   (dkim_alg_t &signalg);
		
		DKIM_STAT     setDKIMHeader  (DKIM *pdkim, const char *phBuf);  //Input Header Message
		DKIM_STAT     setDKIMEOH     (DKIM *pdkim);                     //End Header Message
		DKIM_STAT     setDKIMBody    (DKIM *pdkim, const char *phBuf);  //Input Boby Message
		DKIM_STAT     setDKIMEOM     (DKIM *pdkim);                     //End Body Message

		DKIM_STAT     getSignedHeader(DKIM *pdkim, char * phdr, size_t phdrlen);
		DKIM_STAT     closeDKIM      (DKIM *pdkim);

		dkim_sigkey_t getKeyString   () { return m_pSigKey;};
		
	private:
		CEmsDKIM(){};

		dkim_canon_t  getCanonCode   (const char   *strCanon);
		const  char  *getCanonName   (dkim_canon_t  dkCode);
		dkim_alg_t    getAlgCode     (const char   *pAlgName);
		char *        getSignKey     (const char   *pKeyFilePath);

	private:
		DKIM_LIB      *m_pDKIMLib;    //--ischoi  라이브러리 객체는 단일 객체로하여 포인터로 접근하게하자.
		char          *m_pSelector;   // MX Selector
		char          *m_pDomainName; // MX Domain Name
		char          *m_pSigKeyPath; // dkim key path(unsigned char *)
		dkim_sigkey_t  m_pSigKey;     // dkim key
		dkim_canon_t   m_hdrCanon;    // DKIM_CANON_RELAXED;
		dkim_canon_t   m_bodyCanon;   // DKIM_CANON_SIMPLE;
		dkim_alg_t     m_signAlg;     // DKIM_SIGN_UNKNOWN;

};

#define theDKIMLib() CEmsDKIMLib::getInstance()

#endif  //__CEMS_DKIM_HEADER__
