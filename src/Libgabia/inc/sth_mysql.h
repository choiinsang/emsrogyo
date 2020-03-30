#ifndef	_STH_MYSQL_H_
#define	_STH_MYSQL_H_

#define	USE_OLD_FUNCTIONS
#include <mysql.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

  typedef MYSQL STH_DB;
  typedef MYSQL_RES STH_DB_RES;
  typedef MYSQL_ROW STH_DB_ROW;
//typedef       MYSQL_STMT      STH_DB_STMT;
//typedef       MYSQL_BIND      STH_DB_BIND;

#define	STH_DB_RECONNECT	1
#define	STH_DB_NEWCONNECT	2

#define	DBFetchRow(__res)	mysql_fetch_row(__res)
#define	DBAlive(__db)		mysql_ping(__db)
#define	DBSeek(__res,__idx) mysql_data_seek(__res, __idx)

#define	DBEscapteString(__db,__output,__input,__size)	mysql_real_escape_string(__db,__output,__input,__size)

  char *ConvertSpecialChar (STH_DB * db, const char *szSource,
			    unsigned long sSize);
  int ConnectDB (STH_DB * db, const char *szUser, const char *szPass,
		 const char *szHost, const char *szName, int cType);
  int QueryDB (STH_DB * db, const char *szQuery);
  STH_DB_RES *CreateDBResult (STH_DB * db);
  void CloseResult (STH_DB * db, STH_DB_RES * result);
  void CloseDB (STH_DB * db);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_MYSQL_H_
