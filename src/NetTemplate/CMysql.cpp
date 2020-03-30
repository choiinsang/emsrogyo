#include "CMysql.h"
#include <string.h>
#include <stdio.h>

CMysql::CMysql()
{
	m_bConnected = false;
	m_result = NULL;
}

CMysql::~CMysql()
{
	if( m_bConnected )
	{
		freeResult();
		disconnect();
	}
}

bool CMysql::connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port)
{
	if( m_bConnected ) 
	{
		printf( "mysql::connect() false__already connected\n");
		return false;
	}

	mysql_init(&m_mysql);

	printf("connect -> Host:%s, User:%s, Passwd:%s, Dbname:%s Port:%d\n", host, user, passwd, dbname, port);

	if( !mysql_real_connect(&m_mysql, host, user, passwd, dbname, port, 0, 0) )
	{
		printf( "mysql_real_connect false\n");
		return false;
	}

	printf( "mysql::connect() successed\n");
	m_bConnected = true;	
	return true;
}

bool CMysql::disconnect()
{
	if( !m_bConnected ) return false;
	mysql_close(&m_mysql);
	m_bConnected = false;
	return true;
}

bool CMysql::executeQuery(const char *query, bool bStoreResult)
{
	if( !m_bConnected ) 
	{
		printf("mysql::executeQuery() false__not connected\n");
		return false;
	}	

	freeResult();

	if( 0 != mysql_real_query(&m_mysql, query, strlen(query) ) )
	{
		printf("mysql::executeQuery() false__mysql_real_query() false\n");
		return false;
	}

	if( bStoreResult ) 
		m_result = mysql_store_result(&m_mysql);

	if( !m_result ) 
		return false;

	printf("mysql::executeQuery() successed\n");
	return true;
}

void CMysql::freeResult()
{
	if( m_result )	
	{
		mysql_free_result(m_result);
		m_result = NULL;	
	}
}
