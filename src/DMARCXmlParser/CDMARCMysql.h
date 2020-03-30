#ifndef __CDMARC_MYSQL_HEADER__
#define __CDMARC_MYSQL_HEADER__

#include <string.h>
#include <vector>
#include <list>
#include <stdlib.h>
#include <mysql/mysql.h>

#include <boost/shared_ptr.hpp>

#include "LockObject.h"
#include "DefineNetGlobal.h"
#include "DMARCDefineHeader.h"


#define RowStr(i) MySQLRow[i]!='\0'?MySQLRow[i]:""

using namespace boost;
using namespace std;


class CDMARCMysql : public LockObject
{
public:
	     CDMARCMysql();
	    ~CDMARCMysql();
	
public:
	void  SetHost(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, ULONG l_ulPort);
	bool  ConnectDB();
	void  DisconnectDB();
	
	bool  QueryDB(const char *l_pszQuery);
	bool  SelectDB(const char *l_pszQuery, std::vector<std::string> &l_rvecResult, int *_pRowCnt);
	
	bool  SelectDB_GetResult(const char *l_pszQuery, int *_pRowCnt, int _FieldsCnt);
	void  SelectDB_FreeResult();

public:
	bool  ReconnectDB();    
	bool  CheckAlive();
	bool  Transaction();
	bool  Commit();
	bool  Rollback();
	
	bool  ReconnectDB2(); //9add
	
	bool  CallProc(const char *l_pszQuery, std::vector<std::string> &l_rvecResult);
	int   RowCount(const char *l_pszQuery);
	long long GetSQLInsertID();
	int   MakeUTF8     (const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut);
	int   MakeEUCKR    (const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut);
	int   MakeEUCKRCOPY(const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut);

public:
	bool mIsUseLock;
	
	MYSQL_RES *mpMySQLResult;
	
	char mOwnerName[30];
	void SetOwnerName(char *_Name);

private:
	MYSQL  m_MySQL;
	
	char   m_chHost  [SZ_IP4ADDRESS+1];
	char   m_chUser  [SZ_NAME+1];
	char   m_chPasswd[SZ_PASSWD+1];
	char   m_chDBName[SZ_NAME+1];

	int    m_iPort;
	int    m_iRetry;
	int    m_iRetryPing;

	struct timeval m_CurrTime;
};

extern bool ReplaceQuery (std::string &l_rstrSrc, const char* l_pszSrc, const char* l_pchDest);
extern bool ReplaceQuery (std::string &l_rstrSrc, const char* l_pszSrc, const long long l_llDest);
extern bool ReplaceQuerys(std::string &l_rstrSrc, const char* l_pszSrc, const char* l_pchDest);
extern bool ReplaceQuerys(std::string &l_rstrSrc, const char* l_pszSrc, const long long l_llDest);

#define SelDbType_CpMail		0
#define SelDbType_Campain		1


extern int GetExpResultCode(int _SmtpStep, int _iCode3Ch, char* _StrErrExp=NULL);

#endif   //__CDMARC_MYSQL_HEADER__

