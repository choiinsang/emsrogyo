#include <stdio.h>

#include "EmsMysql.h"
#include <mysql/errmsg.h>
#include <boost/shared_array.hpp>
#include <iconv.h>
#include <unistd.h>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include "EmsDefine.h"
#include "FuncString.h"
#include "EmsLog.h"

using namespace boost;

#define MAX_DBRETRY		3

extern CEmsLog * gLog;

#define SET_EUCKR

CEmsMysql::CEmsMysql():m_iPort(3306), m_iRetry(0), m_iRetryPing(0)
{

	mIsUseLock = false;

	mpMySQLResult = NULL;

	mOwnerName[0] = '\0';
}

CEmsMysql::~CEmsMysql()
{
	try
	{
		DisconnectDB();
	}
	catch (...)
	{
	}
}

void CEmsMysql::SetOwnerName(const char *_Name)
{	
	strcpy(mOwnerName, _Name);
}

//########################################## SetHost ###########################################
void CEmsMysql::SetHost(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, ULONG l_ulPort)
{
//	m_strHost   = l_pszHost;
//	m_strUser   = l_pszUser;
//	m_strPasswd = l_pszPasswd;
//	m_strDBName = l_pszDBName;
	memset(m_chHost,  0, SZ_IP4ADDRESS+1);
	memset(m_chUser,  0, SZ_NAME+1);
	memset(m_chPasswd,0, SZ_PASSWD+1);
	memset(m_chDBName,0, SZ_NAME+1);

	strncpy(m_chHost,  l_pszHost,   SZ_IP4ADDRESS);
	strncpy(m_chUser,  l_pszUser,   SZ_NAME);
	strncpy(m_chPasswd,l_pszPasswd, SZ_PASSWD);
	strncpy(m_chDBName,l_pszDBName, SZ_NAME);

	m_iPort = l_ulPort;
}
//##############################################################################################

//######################################### ConnectDB ##########################################
bool CEmsMysql::ConnectDB()
{
	//MYSQL m_mysql;
//	mysql_init(&m_MySQL);
//
//	printf("connect -> Host:%s, User:%s, Passwd:%s, Dbname:%s Port:%d\n", m_chHost, m_chUser, m_chPasswd,	m_chDBName, m_iPort);
//
//	//if( !mysql_real_connect(&m_mysql, m_chHost, m_chUser, m_chPasswd,	m_chDBName, m_iPort, NULL, CLIENT_MULTI_STATEMENTS) )
//	if( !mysql_real_connect(&m_MySQL, m_chHost, m_chUser, m_chPasswd,	m_chDBName, m_iPort, 0, 0) )
//
//	{
//		printf( "mysql_real_connect false\n");
//		return false;
//	}
//
//	printf( "mysql::connect() successed\n");
//	return true;
	

	bool bRet = false;
	
	if (NULL == mysql_init(&m_MySQL))
	{
		return bRet;
	}

	MYSQL *SQLConn = NULL;

	if(mIsUseLock == true)
		Lock();

	bool bReConnect = true;
	mysql_options( &m_MySQL, MYSQL_OPT_RECONNECT, (char*)&bReConnect);

#ifdef SET_EUCKR
	mysql_options( &m_MySQL, MYSQL_SET_CHARSET_NAME, "euckr"); //추가
	mysql_options( &m_MySQL, MYSQL_INIT_COMMAND, "SET NAMES euckr"); //추가 
#else
	mysql_options( &m_MySQL, MYSQL_SET_CHARSET_NAME, "UTF8"); //추가
	mysql_options( &m_MySQL, MYSQL_INIT_COMMAND, "SET NAMES UTF8"); //추가 
#endif

//	SQLConn = mysql_real_connect(&m_MySQL, m_strHost.c_str(), m_strUser.c_str(), m_strPasswd.c_str(),
//		m_strDBName.c_str(), m_iPort, 0, CLIENT_MULTI_STATEMENTS);
SQLConn = mysql_real_connect(&m_MySQL, m_chHost, m_chUser, m_chPasswd,	m_chDBName, m_iPort, NULL, CLIENT_MULTI_STATEMENTS);

if(mIsUseLock == true)
	Unlock();

	if (SQLConn == NULL)
	{
		gLog->Write("E F[CEmsMysql::ConnectDB] D[connectDB Err] V[(%s)%d.%s]", mOwnerName, mysql_errno(&m_MySQL), mysql_error(&m_MySQL));
	}
	else
	{
#ifdef SET_EUCKR
		QueryDB("SET NAMES euckr");
#else
		QueryDB("SET NAMES UTF8"); //ori
#endif
		QueryDB("SET GLOBAL max_allowed_packet = 1024 * 1024 * 64"); //추가

//#ifdef VERSION_TEST //nanana
//		QueryDB("SET SESSION max_allowed_packet = 1024 * 1024 * 64"); //추가
//#endif
		
		bRet     = true;
		m_iRetry = 0;
	}

	return bRet;
}
//##############################################################################################

//*
bool CEmsMysql::ReconnectDB2()
{
	int nRet;

	if(mIsUseLock == true)
		Lock();

	nRet = mysql_ping(&m_MySQL);

	if(mIsUseLock == true)
		Unlock();

	if (0 != nRet)
	{
		DisconnectDB();
		bool m_bCnt = ConnectDB();

		if(m_bCnt == false)
			return false;

		if(mIsUseLock == true)
			Lock();

		nRet = mysql_ping(&m_MySQL);

		if(mIsUseLock == true)
			Unlock();

		if (0 != nRet)
			return false;
	}

	return true;
}
//*/

//######################################## DisconnectDB ########################################
void CEmsMysql::DisconnectDB()
{
	if (NULL != &m_MySQL)
	{
		if(mIsUseLock == true)
			Lock();

		mysql_close(&m_MySQL);

		if(mIsUseLock == true)
			Unlock();
	}
}
//##############################################################################################

//########################################## QueryDB ###########################################
bool CEmsMysql::QueryDB(const char *l_pszQuery)
{
	//totoro SM_D pLog->Write("D F[CEmsMysql::QueryDB] D[Start] V[Owner:%s, query:%s]", mOwnerName, l_pszQuery);

	bool bResult(false);
	int  nRet(0);	

#ifdef SET_EUCKR
#else
	size_t size_In, size_Out;
	size_In  = strlen(l_pszQuery);
	size_Out = size_In * 2 + 10;
	boost::shared_array<char> szOutBuff = boost::shared_array<char>(new char[size_Out]);
	memset( szOutBuff.get(), 0x00, size_Out);
	nRet = MakeUTF8(l_pszQuery, size_In, szOutBuff.get(), size_Out);
#endif	

	if (nRet >= 0)
	{
		// Query 실행. 성공시 0을 리턴함

		if(mIsUseLock == true)
			Lock();

#ifdef SET_EUCKR
		nRet = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
		nRet = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif		

		if(mIsUseLock == true)
			Unlock();

		if (0 == nRet)
		{
			bResult = true;
			//            DebugLog((DEB_DEBUG, "QUERY[%s] \n", pQuery));
		}
		else
		{
			gLog->Write("E F[CEmsMysql::QueryDB] D[(%s)query:%s] E[] V[%s.%d][mysql_real_query] error(%s)(%d)", mOwnerName, l_pszQuery, __FILE__, __LINE__, mysql_error(&m_MySQL), mysql_errno(&m_MySQL));
			//DebugLog((DEB_ERROR, "[%s.%d][mysql_real_query] error(%s)(%d) \n", __FILE__, __LINE__, mysql_error(&m_MySQL), mysql_errno(&m_MySQL)));
			//if (true == ReconnectDB())
			//    return QueryDB(l_pszQuery);

			if(ReconnectDB2() == false)
			{
				gLog->Write("E F[CEmsMysql::QueryDB] D[(%s)query:%s] E[ReconnectDB2 Failed]", mOwnerName, l_pszQuery);
				//exit(0);  //--ischoi--0630 임시
				return false;
			}
			else
			{
				gLog->Write("D F[CEmsMysql::QueryDB] D[(%s)query:%s] E[ReconnectDB2 OK]", mOwnerName, l_pszQuery);
			}

			if(mIsUseLock == true)
				Lock();

#ifdef SET_EUCKR
			nRet = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
			nRet = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif		

			if(mIsUseLock == true)
				Unlock();

			if (0 == nRet)
			{
				bResult = true;
				//            DebugLog((DEB_DEBUG, "QUERY[%s] \n", pQuery));
			}
			else
			{
				gLog->Write("E F[CEmsMysql::QueryDB] D[(%s)query:%s] E[] V[%s.%d][mysql_real_query2] error(%s)(%d)", mOwnerName, l_pszQuery, __FILE__, __LINE__, mysql_error(&m_MySQL), mysql_errno(&m_MySQL));
				//exit(0);  //--ischoi--0630 임시
				return false;
			}
		}
	}

	return bResult;
}
//##############################################################################################

//########################################## SelectDB ##########################################
bool CEmsMysql::SelectDB(const char *l_pszQuery, vector<string> &l_rvecResult, int *_pRowCnt)
{
	int  iResult(0);
	int  iFields;
	int  iRows;
	int  i(0);
	int  nRet(0);
	bool bResult(false);	

#ifdef SET_EUCKR
#else
	size_t  size_In, size_Out;
	size_In  = strlen(l_pszQuery);
	size_Out = size_In * 2 + 10;
	boost::shared_array<char> szOutBuff = boost::shared_array<char>(new char[size_Out]);
	memset( szOutBuff.get(), 0x00, size_Out);
	nRet = MakeUTF8(l_pszQuery, size_In, szOutBuff.get(), size_Out);
#endif	

	MYSQL_RES *pMySQLResult;
	MYSQL_ROW MySQLRow;

	//    DebugLog((DEB_TRACE, "Query, %s \n", l_pszQuery));
	if (nRet >= 0)
	{
		if(mIsUseLock == true)
			Lock();

#ifdef SET_EUCKR
		iResult = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
		iResult = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif

		if (0 == iResult)
		{ // 성공
			pMySQLResult = mysql_store_result(&m_MySQL);

			if (pMySQLResult)  // 리턴된 ROW가 있을 경우
			{
				iRows = mysql_num_rows(pMySQLResult); // ROW의 수

				if(_pRowCnt)
					*_pRowCnt = iRows;

				if (0 == iRows)   // 질의는 성공했으나, 얻어진 값이 하나도 없을 경우
				{
					//                l_rvecResult.push_back("");
					mysql_free_result(pMySQLResult);
				}
				else
				{
					iFields  = mysql_num_fields(pMySQLResult);  // FIELDS의 수 
					l_rvecResult.reserve(iRows * iFields);

					while((MySQLRow = mysql_fetch_row(pMySQLResult)))
					{
						//                      DebugLog((DEB_DEBUG, "[%s.%d] FIELDS(%d) ROWS(%d) ROW[0] = %s \n", __FILE__, __LINE__,
						//                        iFields, iRows, MySQLRow[0]));
						for(i = 0; i < iFields; i++)
						{
							if ('\0' != MySQLRow[i])
							{
#ifdef SET_EUCKR
								l_rvecResult.push_back(MySQLRow[i]);
#else
								size_In  = strlen(MySQLRow[i]);
								size_Out = size_In + 1;
								boost::shared_array<char> szOutBuff2 = boost::shared_array<char>(new char[size_Out]);
								//save memset( szOutBuff2.get(), 0x00, size_Out);
								nRet = MakeEUCKR(MySQLRow[i], size_In, szOutBuff2.get(), size_Out);
								l_rvecResult.push_back(szOutBuff2.get());
#endif
							}
							else
							{
								//                                DebugLog((DEB_ERROR, "[%s.%d] ROW[0] '%c' \n", __FILE__, __LINE__,
								//                                  MySQLRow[i]));
								l_rvecResult.push_back("");
							}
						}
					}
					mysql_free_result(pMySQLResult);
				}

				bResult = true;
			}
			else
			{
				gLog->Write("E F[CEmsMysql::SelectDB] D[query:%s] E[] V[%s.%d][mysql_real_query] error(%s)(%d)", l_pszQuery, __FILE__, __LINE__, mysql_error(&m_MySQL), mysql_errno(&m_MySQL));
			}

			if(mIsUseLock == true)
				Unlock();
		}
		else
		{
			gLog->Write("E F[CEmsMysql::SelectDB] D[query:%s] E[0 == iResult] [mysql_real_query]", l_pszQuery);

			if(mIsUseLock == true)
				Unlock();
			//            if (true == ReconnectDB())
			//                return SelectDB(l_pszQuery, l_rvecResult);

		}
	}

	return bResult;
}
//##############################################################################################

void CEmsMysql::SelectDB_FreeResult()
{
	if(mpMySQLResult)
	{
		mysql_free_result(mpMySQLResult);
		mpMySQLResult = NULL;
	}

	if(mIsUseLock == true)
		Unlock();
}

bool CEmsMysql::SelectDB_GetResult(const char *l_pszQuery, int *_pRowCnt, int _FieldsCnt)
{
	//totoro SM_D pLog->Write("D F[CEmsMysql::SelectDB_GetResult] D[Start] V[Owner:%s, query:%s]", mOwnerName, l_pszQuery);

	int  iResult(0);
	int  iFields;
	int  iRows;
	//int  i(0);
	int  nRet(0);
	//bool bResult(false);	

#ifdef SET_EUCKR
#else
	size_t  size_In, size_Out;
	size_In  = strlen(l_pszQuery);
	size_Out = size_In * 2 + 10;
	boost::shared_array<char> szOutBuff = boost::shared_array<char>(new char[size_Out]);
	memset( szOutBuff.get(), 0x00, size_Out);
	nRet = MakeUTF8(l_pszQuery, size_In, szOutBuff.get(), size_Out);
#endif	

	//MYSQL_RES *pMySQLResult;
	//MYSQL_ROW MySQLRow;

	if (nRet >= 0)
	{
	}
	else
	{
		gLog->Write("E F[CEmsMysql::SelectDB_GetResult] E[MakeUTF8 failed] V[(%s)]", mOwnerName);
		return false;
	}

	if(mpMySQLResult != NULL)
	{
		gLog->Write("E F[CEmsMysql::SelectDB_GetResult] E[mpMySQLResult != NULL] V[(%s)]", mOwnerName);
		//exit(0);  //--ischoi--0630 임시
		return false;
	}

	if(mIsUseLock == true)
		Lock();


#ifdef SET_EUCKR
	iResult = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
	iResult = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif

	if (iResult != 0) //0이성공
	{
		gLog->Write("E F[CEmsMysql::SelectDB_GetResult] D[(%s)query:%s] E[0 == iResult] [mysql_real_query]", mOwnerName, l_pszQuery);

		if(mIsUseLock == true)
			Unlock();
		//            if (true == ReconnectDB())
		//                return SelectDB(l_pszQuery, l_rvecResult);

		if(ReconnectDB2() == false)
		{
			gLog->Write("E F[CEmsMysql::SelectDB_GetResult] D[(%s)query:%s] E[ReconnectDB2 Failed]", mOwnerName, l_pszQuery);
			return false;
		}
		else
		{
			gLog->Write("D F[CEmsMysql::SelectDB_GetResult] D[(%s)query:%s] E[ReconnectDB2 OK]", mOwnerName, l_pszQuery);
		}

		if(mIsUseLock == true)
			Lock();

#ifdef SET_EUCKR
		iResult = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
		iResult = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif

		if (iResult != 0) //0이성공
		{
			if(mIsUseLock == true)
				Unlock();

			//exit(0);  //--ischoi--0630 임시
			return false;
		}
		else
		{
			
		}

		//return false;
	}

	mpMySQLResult = mysql_store_result(&m_MySQL);
	//fprintf(stdout,  "[%s][%s][%d] mysql_store_result [%d]\n", __FILE__, __FUNCTION__, __LINE__, mpMySQLResult);

	if (mpMySQLResult == NULL)
	{
		gLog->Write("E F[CEmsMysql::SelectDB_GetResult] D[(%s)query:%s] E[] V[%s.%d][mysql_real_query] error(%s)(%d)", mOwnerName, l_pszQuery, __FILE__, __LINE__, mysql_error(&m_MySQL), mysql_errno(&m_MySQL));

		if(mIsUseLock == true)
			Unlock();

		return false;
	}

	iRows = mysql_num_rows(mpMySQLResult); // ROW의 수
	//fprintf(stdout,  "[%s][%s][%d] mysql_num_rows [%d]\n", __FILE__, __FUNCTION__, __LINE__, iRows);

	if(_pRowCnt)
		*_pRowCnt = iRows;

	if (iRows == 0)   // 질의는 성공했으나, 얻어진 값이 하나도 없을 경우
	{
		//                l_rvecResult.push_back("");
		mysql_free_result(mpMySQLResult);
		mpMySQLResult = NULL;

		if(mIsUseLock == true)
			Unlock();

		return true;
	}
	else
	{
		//mpMySQLResult =  pMySQLResult;

		iFields = mysql_num_fields(mpMySQLResult);  // FIELDS의 수
		
		if(_FieldsCnt != iFields)
		{
			mysql_free_result(mpMySQLResult);
			mpMySQLResult = NULL;

			gLog->Write("E F[CEmsMysql::SelectDB_GetResult] D[(%s)query:%s] E[_FieldsCnt(%d) != iFields(%d)]", mOwnerName, l_pszQuery, _FieldsCnt, iFields);

			if(mIsUseLock == true)
				Unlock();

			return false;
		}
	}

	//bResult = true;

	if(mIsUseLock == true)
		Unlock();

	return true;
}


bool CEmsMysql::ReconnectDB()
{
//	int    iDBErr(mysql_errno(&m_MySQL));
	ULONG  ulTimeDiff;
	struct timeval CurrTime;
	gettimeofday(&CurrTime, NULL);
	ulTimeDiff = ( CurrTime.tv_sec - m_CurrTime.tv_sec ) * 1000 +
		( CurrTime.tv_usec - m_CurrTime.tv_usec ) / 1000;

	if (MAX_DBRETRY < m_iRetry++)
	{
		//gLog->Write("D F[CEmsMysql::ReconnectDB] ReconnectDB Err[%d] Retry Count Over & WaitTime[%ld]\n", iDBErr, ulTimeDiff);
		if (ulTimeDiff > 30000)
			m_iRetry = 0;
		else
			return false;
	}
	gettimeofday(&m_CurrTime, NULL);

	//    DebugLog((DEB_TRACE, "[%s.%d][MYSQL ERROR(%d)] \n", __FILE__, __LINE__, iDBErr));
	//gLog->Write("D F[CEmsMysql::ReconnectDB] ReconnectDB Err[%d] Retry[%d]\n", iDBErr, m_iRetry);
	DisconnectDB();
	return ConnectDB();
}

bool CEmsMysql::CheckAlive()
{
	bool bResult(true);
	int  nRet(0);

	if(mIsUseLock == true)
		Lock();

	nRet = mysql_ping(&m_MySQL);

	if(mIsUseLock == true)
		Unlock();

	if (0 != nRet)
	{
		if (MAX_DBRETRY < m_iRetryPing++)
		{
			bResult = ReconnectDB();
			m_iRetryPing = 0;
		}
	}
	else
	{
		m_iRetryPing = 0;
	}

	return bResult;
}

int CEmsMysql::RowCount(const char *l_pszQuery)
{
	int iResult(0);
	int iRows(-1);

	int nRet(0);


#ifdef SET_EUCKR
#else
	size_t  size_In, size_Out;
	size_In  = strlen(l_pszQuery);
	size_Out = size_In * 2 + 10;
	boost::shared_array<char> szOutBuff = boost::shared_array<char>(new char[size_Out]);
	memset( szOutBuff.get(), 0x00, size_Out);
	nRet = MakeUTF8(l_pszQuery, size_In, szOutBuff.get(), size_Out);
#endif	

	if (nRet >= 0)
	{
		MYSQL_RES *pMySQLResult;

		if(mIsUseLock == true)
			Lock();

#ifdef SET_EUCKR
		iResult = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
		iResult = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif		

		if (0 == iResult) // 성공 : 0 
		{
			pMySQLResult = mysql_store_result(&m_MySQL); // 리턴된 row가 있다.
			if (pMySQLResult)
			{
				iRows = mysql_num_rows(pMySQLResult);
				//                DebugLog((DEB_TRACE, "[%s.%d] %s - rowCount(%d) \n", __FILE__, __LINE__, l_pszQuery, iRows));
			}
			//            else
			//            {
			//                DebugLog((DEB_ERROR, "[%s.%d][mysql_store_result] error(%s)(%d) \n", __FILE__, __LINE__,
			//                  mysql_error(&m_MySQL), mysql_errno(&m_MySQL)));
			//            }
			mysql_free_result(pMySQLResult);

			if(mIsUseLock == true)
				Unlock();
		}
		else
		{
			if(mIsUseLock == true)
				Unlock();
			//            if (true == ReconnectDB())
			//                return RowCount(l_pszQuery);
			//            else
			return -1;
		}
	}

	return iRows;
}


bool CEmsMysql::CallProc(const char *l_pszQuery, vector<string> &l_rvecResult)
{
	int  iResult(0), iFields, iRows, i = 0;
	int  nRet(0);
	bool bResult(false);	

#ifdef SET_EUCKR
#else
	size_t  size_In, size_Out;
	size_In  = strlen(l_pszQuery);
	size_Out = size_In * 2 + 10;
	boost::shared_array<char> szOutBuff = boost::shared_array<char>(new char[size_Out]);
	memset( szOutBuff.get(), 0x00, size_Out);
	nRet = MakeUTF8(l_pszQuery, size_In, szOutBuff.get(), size_Out);
#endif

	MYSQL_RES *pMySQLResult;
	MYSQL_ROW MySQLRow;

	//    DebugLog((DEB_TRACE, "Query, %s \n", l_pszQuery));
	if (nRet >= 0)
	{
		if(mIsUseLock == true)
			Lock();

#ifdef SET_EUCKR
		iResult = mysql_real_query(&m_MySQL, l_pszQuery, strlen(l_pszQuery));
#else
		iResult = mysql_real_query(&m_MySQL, szOutBuff.get(), strlen(szOutBuff.get()));
#endif		

		do
		{
			pMySQLResult = mysql_store_result(&m_MySQL);
			if (pMySQLResult) // 리턴된 ROW가 있을 경우
			{
				iRows = mysql_num_rows(pMySQLResult); // ROW의 수
				if (0 == iRows) // 질의는 성공했으나, 얻어진 값이 하나도 없을 경우
				{
					//                    l_rvecResult.push_back("");
					//                      mysql_free_result(pMySQLResult);
				}
				else
				{
					iFields = mysql_num_fields(pMySQLResult);
					l_rvecResult.reserve(iRows * iFields);
					while((MySQLRow = mysql_fetch_row(pMySQLResult)))
					{
						for(i = 0; i < iFields; i++)
						{
							if ('\0' != MySQLRow[i])
							{
#ifdef SET_EUCKR
								l_rvecResult.push_back(MySQLRow[i]);
#else
								//nRet = MakeEUCKR(MySQLRow[i], size_In, szOutBuff2.get(), size_Out);
								size_In  = strlen(MySQLRow[i]);
								size_Out = size_In + 1;
								boost::shared_array<char> szOutBuff2 = boost::shared_array<char>(new char[size_Out]);
								//save memset( szOutBuff2.get(), 0x00, size_Out);
								nRet = MakeEUCKR(MySQLRow[i], size_In, szOutBuff2.get(), size_Out);
								l_rvecResult.push_back(szOutBuff2.get());
#endif
								
							}
							else
							{
								l_rvecResult.push_back("");
							}
						}
					}
				}
				mysql_free_result(pMySQLResult);
				bResult = true;
			}
			else
			{
				if (mysql_field_count(&m_MySQL) == 0)
				{
					bResult = true;
				}
				else
				{
					break;
				}
			}

			iResult = mysql_next_result(&m_MySQL);
		} while (iResult == 0);

		if(mIsUseLock == true)
			Unlock();
	}

	return bResult;
}

bool CEmsMysql::Transaction()
{
	return QueryDB("START TRANSACTION");
}

bool CEmsMysql::Commit()
{
	return QueryDB("COMMIT");
}

bool CEmsMysql::Rollback()
{
	return QueryDB("ROLLBACK");
}

long long CEmsMysql::GetSQLInsertID()
{
	return mysql_insert_id(&m_MySQL);
}

int CEmsMysql::MakeUTF8(const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut)
{
	int nRet(0);
	boost::shared_array<char> szInBuff = boost::shared_array<char>(new char[l_SizeIn + 1]);

	strcpy(szInBuff.get(), l_pszIn);

	char* pszInBuff = szInBuff.get();

	iconv_t it;

	//    it   = iconv_open( "UTF-8", "EUC-KR");
	it   = iconv_open( "UTF-8", "CP949");
	nRet = iconv( it, &pszInBuff, &l_SizeIn, &l_pszOut, &l_SizeOut );
	iconv_close( it );

	return nRet;
}

int CEmsMysql::MakeEUCKR(const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut)
{
	int nRet(0);
	boost::shared_array<char> szInBuff = boost::shared_array<char>(new char[l_SizeIn + 1]);

	strcpy(szInBuff.get(), l_pszIn);

	char* pszInBuff = szInBuff.get();

	iconv_t it;

	//    it   = iconv_open( "EUC-KR", "UTF-8");
	it   = iconv_open( "CP949", "UTF-8");
	nRet = iconv( it, &pszInBuff, &l_SizeIn, &l_pszOut, &l_SizeOut );
	iconv_close( it );

	return nRet;
}

int CEmsMysql::MakeEUCKRCOPY(const char *l_pszIn, size_t l_SizeIn, char *l_pszOut, size_t l_SizeOut)
{
	strncpy(l_pszOut, l_pszIn, l_SizeIn);
	l_pszOut[l_SizeIn] = '\0';
	return l_SizeIn;
}


bool ReplaceQuery(std::string &l_rstrSrc, const char* l_pszSrc, const char* l_pchDest)
{
	int iSize(strlen(l_pszSrc));
	std::string strTemp = l_rstrSrc;
	std::string::size_type posFirst = strTemp.find(l_pszSrc);
	std::string::size_type posLast  = posFirst + iSize;


	if (std::string::npos != posFirst && std::string::npos != posLast)
	{
		l_rstrSrc.assign(strTemp.c_str(), posFirst);
		l_rstrSrc.append(l_pchDest);
		l_rstrSrc.append(strTemp.substr(posFirst + iSize, strTemp.size() - posFirst - iSize));
	}
	return true;
}

bool ReplaceQuery(std::string &l_rstrSrc, const char* l_pszSrc, const long long l_llDest)
{
	int iSize(strlen(l_pszSrc));
	std::string strTemp = l_rstrSrc;
	std::string::size_type posFirst = strTemp.find(l_pszSrc);
	std::string::size_type posLast  = posFirst + iSize;

	if (std::string::npos != posFirst && std::string::npos != posLast)
	{
		l_rstrSrc.assign(strTemp.c_str(), posFirst);
		l_rstrSrc.append(boost::lexical_cast<std::string>(l_llDest));
		l_rstrSrc.append(strTemp.substr(posFirst + iSize, strTemp.size() - posFirst - iSize));
	}
	return true;
}

bool ReplaceQuerys(std::string &l_rstrSrc, const char* l_pszSrc, const char* l_pchDest)
{
	int iSize(strlen(l_pszSrc));
	std::string strTemp = l_rstrSrc;
	std::string::size_type posFirst(strTemp.find(l_pszSrc));
	std::string::size_type posLast(posFirst + iSize);

	while(std::string::npos != posFirst)
	{
		l_rstrSrc.assign(strTemp.c_str(), posFirst);
		l_rstrSrc.append(l_pchDest);
		l_rstrSrc.append(strTemp.substr(posFirst + iSize, strTemp.size() - posFirst - iSize));
		strTemp = l_rstrSrc;

		posFirst = strTemp.find(l_pszSrc, posFirst);
		posLast  = posFirst + iSize;
	}

	return true;
}

bool ReplaceQuerys(std::string &l_rstrSrc, const char* l_pszSrc, const long long l_llDest)
{
	int iSize(strlen(l_pszSrc));
	std::string strTemp = l_rstrSrc;
	std::string::size_type posFirst = strTemp.find(l_pszSrc);
	std::string::size_type posLast  = posFirst + iSize;

	while(std::string::npos != posFirst)
	{
		l_rstrSrc.assign(strTemp.c_str(), posFirst);
		l_rstrSrc.append(boost::lexical_cast<std::string>(l_llDest));
		l_rstrSrc.append(strTemp.substr(posFirst + iSize, strTemp.size() - posFirst - iSize));
		strTemp = l_rstrSrc;

		posFirst = strTemp.find(l_pszSrc, posFirst);
		posLast  = posFirst + iSize;
	}
	return true;
}


////---------------------------------
//// class CEmsMysqlConnSet
////---------------------------------
//CEmsMysqlConnSet::CEmsMysqlConnSet()
//: m_bConnected  (false)
//, m_DBName      (NULL)
//{
//	time(&m_tLastUseTime);
//}
//	
//CEmsMysqlConnSet::CEmsMysqlConnSet(const char *db_name)
//: m_bConnected  (false)
//{
//	m_DBName = strdup(db_name);
//}
//	
//CEmsMysqlConnSet::~CEmsMysqlConnSet()
//{
//	m_bConnected = false;
//	if(m_DBName != NULL)
//		free(m_DBName);
//}
//
//bool CEmsMysqlConnSet::connectDB(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, ULONG l_ulPort)
//{
//	do{
//		std::string strConnName = l_pszDBName;
//		m_MysqlCon.get()->SetOwnerName(strConnName.c_str());
//		m_MysqlCon.get()->SetHost(l_pszHost, l_pszUser, l_pszPasswd, strConnName.c_str(), l_ulPort);
//		m_bConnected = m_MysqlCon.get()->ConnectDB();
//		
//		if(m_bConnected == false)
//			break;
//	
//		strConnName += "Sub";
//		m_MysqlConSub.get()->SetOwnerName(strConnName.c_str());
//		m_MysqlConSub.get()->SetHost(l_pszHost, l_pszUser, l_pszPasswd, strConnName.c_str(), l_ulPort);
//		m_bConnected = m_MysqlConSub.get()->ConnectDB();
//		break;
//	}while(1);
//
//	return m_bConnected;
//}
//
//

