#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CThreadPool.h"
#include "CServer.h"
#include "CMysql.h"
#include "mysql/errmsg.h"
#include <string>
using namespace std;

#define MAX_THREADS	10

void addQueue(shared_ptr<string> item);

class CMyThread : public CThread<string>
{
private:
	shared_ptr<CMysql> m_pMysql;
	string m_host, m_user, m_passwd, m_dbname;
	int m_port;

public:
	CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < string > > > pQueue);
	void processRequest(shared_ptr<string> pRequest);
	bool connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port);
};

CMyThread::CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < string > > > pQueue)
: CThread<string>(identifier, pMutex, pCond, pQueue)
{
	printf("sub=>00 %d\n", identifier);
	m_pMysql = shared_ptr<CMysql>(new CMysql);
}

bool CMyThread::connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port)
{
	m_host = host;
	m_user = user;
	m_passwd = passwd;
	m_dbname = dbname;
	m_port = port;

	return m_pMysql->connect(m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), m_port);
}

void CMyThread::processRequest(shared_ptr<string> pRequest)
{
	while( !m_pMysql->executeQuery( pRequest->c_str(), true ) )	
	{
		printf("!!processRequest entered, & failed!! ---> %d\n", m_identifier);
		int mysql_err = mysql_errno(m_pMysql->getMysql());
		int count = 0;
		switch( mysql_err )
		{
		case CR_CONN_HOST_ERROR: 
		case CR_SERVER_GONE_ERROR:
			m_pMysql->disconnect();
			while( !m_pMysql->connect(m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), m_port) )
			{
				//				printf("Failed to connect database as followings : %s, %s, %s, %s, %d\n",
				//					m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), m_port);
				count++;
				if( 3 == count )
				{
					addQueue(pRequest);
				}
				sleep(5);
			}
			if( count > 3 ) return;
			break;
		default:
			return;
		}
	}

	MYSQL_RES *pRes = m_pMysql->getResult();

	if( NULL == pRes ) 
	{
		printf("Woops!\n");
		return;
	}

	MYSQL_ROW row;

	while( row = mysql_fetch_row(pRes) )
	{
		printf("%s %s %s\n", row[0], row[1], row[2]);
	}

	m_pMysql->freeResult();
}

shared_ptr< CThreadPool<string, CMyThread> > tp;

int main()
{
	printf("cur=>00\n");

	tp = (shared_ptr< CThreadPool<string, CMyThread> >)
		(new CThreadPool<string, CMyThread>(MAX_THREADS));

	printf("cur=>01\n");

	for(int i=0; i<MAX_THREADS; i++)
	{
		if( 0 == ( ( i / 2.0 ) - ( i / 2 ) ) )
		{
			//tp->getThread(i)->connect("222.231.60.233", "root", "jj0215", "mysql", 3307);	
			//tp->getThread(i)->connect("121.254.168.207", "hsj", "gabia001", "dbhsj", 3306);	
			tp->getThread(i)->connect("localhost", "hsj", "gabia001", "dbhsj", 3306);	
		}
		else
		{
			//tp->getThread(i)->connect("222.231.60.234", "root", "jj0215", "mysql", 3307);	
			//tp->getThread(i)->connect("121.254.168.207", "hsj", "gabia001", "dbhsj", 3306);	
			tp->getThread(i)->connect("localhost", "hsj", "gabia001", "dbhsj", 3306);	
		}
	}

	printf("cur=>02\n");
	//return 0;

	int wait = 0;
	shared_ptr<string> pi;
	int count = 0;
	srand(time(NULL));

	for(;;)
	{
		shared_ptr<string>pi = shared_ptr<string>(new string("SELECT host, user, password from user"));
		tp->addQueue(pi);
		wait = rand() % 100;
		printf("main - push %s, wait for %.2f sec\n", pi->c_str(), (double)wait / 100);
		usleep(wait * 10000);
	}
}







