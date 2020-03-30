#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "sth_mysql.h"
#include "sth_syslog.h"

char *
ConvertSpecialChar( STH_DB *db, const char *szSource, unsigned long sSize )
{
	char *ret;
	unsigned long lAssign=0;
/*
	if( mysql_ping(db) != 0 )
	{
		Trace(TR_ERROR,"Database Not connect");
		return NULL;
	}
*/
	if( sSize <= 0 || szSource == NULL )
	{
		Trace(TR_ERROR,"ConvertCharacter source null");
		return NULL;
	}

	ret = (char *)Malloc(sSize*2);
	lAssign = mysql_real_escape_string( db, ret, szSource,sSize );

	if( lAssign > sSize*2 || lAssign <= 0 )
	{
		Trace(TR_ERROR,"ConvertCharacter ERROR");
		free(ret);
		return NULL;
	}

	*(ret+lAssign)=0;
	return ret;
}

int
ConnectDB( STH_DB *db, const char *szUser, const char *szPass, const char *szHost, const char *szName, int cType)
{
	if( cType == STH_DB_RECONNECT || ( cType == STH_DB_NEWCONNECT && mysql_ping(db) != 0 ) )
	{
		if( mysql_ping(db) == 0 ) mysql_close(db);
		mysql_init(db);
		mysql_real_connect( db, szHost, szUser, szPass, szName, 0, NULL, 0 );

		if( mysql_ping(db) != 0 )
		{
			Trace(TR_ERROR,"Can't connect Database [%s] [%s] [%s]",
				szHost, szUser, szPass);
			return FALSE;
		}
	}

	return TRUE;
}

int
QueryDB( STH_DB *db,const char *szQuery )
{
	if( mysql_ping(db) != 0 )
	{
		Trace(TR_ERROR,"Database Not connect");
		return FALSE;
	}

	if(mysql_query(db,szQuery) != 0 )
	{
		Trace(TR_ERROR,"SQL Statement error [%s]",szQuery);
		return FALSE;
	}

	return TRUE;
}

STH_DB_RES *
CreateDBResult(STH_DB *db)
{
	STH_DB_RES *ret;

	if( (ret = mysql_store_result( db ) ) == NULL )
	{
		Trace(TR_ERROR,"SQL Statement create failed");
		return NULL;
	}

	return ret;
}

void
CloseResult(STH_DB *db, STH_DB_RES *result)
{
	if( result )
	{
		mysql_free_result(result);
	}
}

void
CloseDB(STH_DB *db)
{
	if( mysql_ping(db) != 0 )
	{
		mysql_close(db);
	}
}
