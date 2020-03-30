#ifndef __CMYSQL__
#define	__CMYSQL__

#include "mysql/mysql.h"

#ifndef	NULL	
#define	NULL	0
#endif

class CMysql 
{
private:
	MYSQL m_mysql;
	bool m_bConnected;
	MYSQL_RES *m_result;

public:
	CMysql();
	~CMysql();

	bool connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port);
	bool disconnect();

	bool executeQuery(const char *query, bool bStoreResult);
	MYSQL_RES *getResult() { return m_result; };
	void freeResult();

	MYSQL *getMysql() { return &m_mysql; };
};

#endif
