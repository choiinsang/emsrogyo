#ifndef  __DMARC_HEADER_DEFINE__HEADER
#define  __DMARC_HEADER_DEFINE__HEADER

#include "../NetTemplate/DefineNetGlobal.h"


const char DMARCXMLPARSER_VERSION     [] = "0.1.0";
const char DMARCXMLPARSER_SUB_VERSION [] = "RQ1";
const char DMARCXMLPARSER_RELEASE_DATE[] = "2015.12.14";
const char DMARCXMLPARSER_INI         [] = "dmarcxmlparser.ini";

//[DB Define]
const int INIT_DEFAULT_DBPORT       = 3306;

//[Size Define]

const unsigned int SZ_TMPBUFFER     = 512;
const unsigned int SZ_NAME          = 255;
const unsigned int SZ_PASSWD        = 255;

const unsigned int SZ_IP4ADDRESS    = 15;
const unsigned int SZ_TEMP          = 1024;
const unsigned int SZ_DEFAULTBUFFER = 4096;

const unsigned int LENGTH           = 0x1000; // 4096

//[THREAD STATE DEFINE]
enum _THREADSTATE{
	TSTATE_PREREADY = 0,
	TSTATE_READY    = 1,
	TSTATE_RUN      = 2,
	TSTATE_PAUSE    = 3,
	TSTATE_STOP     = 4
};

const char DMARC_FEEDBACK   [] = "feedback";
//-------------------------------------------------------
const char DMARC_REPORT_METADATA                   [] = "report_metadata";

const char DMARC_REPORT_METADATA_ORG_NAME          [] = "org_name";
const char DMARC_REPORT_METADATA_EMAIL             [] = "email";
const char DMARC_REPORT_METADATA_EXTRA_CONTACT_INFO[] = "extra_contact_info";
const char DMARC_REPORT_METADATA_REPORT_ID         [] = "report_id";
const char DMARC_REPORT_METADATA_DATE_RANGE        [] = "date_range";
const char DMARC_REPORT_METADATA_DATE_RANGE_BEGIN  [] = "begin";
const char DMARC_REPORT_METADATA_DATE_RANGE_END    [] = "end";

//-------------------------------------------------------
const char DMARC_POLICY_PUBLISHED                  [] = "policy_published";
const char DMARC_POLICY_PUBLISHED_DOMAIN           [] = "domain";
const char DMARC_POLICY_PUBLISHED_ADKIM            [] = "adkim";
const char DMARC_POLICY_PUBLISHED_ASPF             [] = "aspf";
const char DMARC_POLICY_PUBLISHED_P                [] = "p";
const char DMARC_POLICY_PUBLISHED_SP               [] = "sp";
const char DMARC_POLICY_PUBLISHED_PCT              [] = "pct";

//-------------------------------------------------------
const char DMARC_RECORD                            [] = "record";
                                                   
const char DMARC_RECORD_ROW                        [] = "row";
const char DMARC_RECORD_ROW_SOURCE_IP              [] = "source_ip";
const char DMARC_RECORD_ROW_COUNT                  [] = "count";
const char DMARC_RECORD_ROW_POLICY_EVALUATED       [] = "policy_evaluated";
const char DMARC_RECORD_ROW_P_DISPOSITION          [] = "disposition";
const char DMARC_RECORD_ROW_P_DKIM                 [] = "dkim";
const char DMARC_RECORD_ROW_P_SPF                  [] = "spf";

const char DMARC_RECORD_IDENTIFIRES                [] = "identifiers";
const char DMARC_RECORD_I_HEADER_FROM              [] = "header_from";
const char DMARC_RECORD_AUTH_RESULTS               [] = "auth_results";
const char DMARC_RECORD_AUTH_RESULTS_SPF           [] = "spf";
const char DMARC_RECORD_AUTH_RESULTS_SPF_DOMAIN    [] = "domain";
const char DMARC_RECORD_AUTH_RESULTS_SPF_RESULT    [] = "result";
const char DMARC_RECORD_AUTH_RESULTS_DKIM          [] = "dkim";
const char DMARC_RECORD_AUTH_RESULTS_DKIM_DOMAIN   [] = "domain";
const char DMARC_RECORD_AUTH_RESULTS_DKIM_RESULT   [] = "result";



#endif //__DMARC_HEADER_DEFINE__HEADER
