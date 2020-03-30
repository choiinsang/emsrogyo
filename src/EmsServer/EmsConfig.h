#ifndef __CEMS_CONFIG_HEADER__
#define __CEMS_CONFIG_HEADER__


#include <iostream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <sys/time.h>

#include "../Common/INIFile.h"
#include "../NetTemplate/LockObject.h"

#include "EmsDefine.h"

using namespace std;

//[RABBITMQ_INIT]
typedef struct st_RABBITMQ_INFO_t {
	char   _chRabbitmq_HostName[SZ_NAME+1];
	char   _chRabbitmq_IP      [SZ_IP4ADDRESS+1];
	int    _iRabbitmq_Port;
	char   _chRabbitmq_User    [SZ_NAME+1];
	char   _chRabbitmq_Passwd  [SZ_PASSWD+1];
	char   _chRabbitmq_Vhost   [SZ_NAME+1];
} stRabbitmqInfo;


//[DKIM_INIT]
typedef struct st_DKIMINFO_t {
	int    _iDKIMSignAlg;    
	char   _chDKIMKeyPath    [SZ_NAME+1];
	char   _chDKIMSelector   [SZ_NAME+1];
	char   _chDKIMDomainName [SZ_NAME+1];
	char   _chDKIMHeaderCanon[SZ_NAME+1];
	char   _chDKIMBodyCanon  [SZ_NAME+1];
} stDKIMInfo;

class CEMSConfig : public LockObject
{
	// Construction
public:
	~CEMSConfig();


private:
	
	CEMSConfig();
	// Overrides

public:
	// Implementation
	bool        InitConfig         (const char * l_pFileName);
	const char *checkString        (char * pStr);

	//[INIT]                        
	const char *getDNSFileName     ();
	int         getDNSRefreshTime  () { return m_iDNSRefreshTimeInterval; }
	int         getMAXMXRetryCount () { return m_iMAXMXRetryCount;        }

	//[DBINFO]                      
	const char *getDBName          ();
	const char *getDBUser          ();
	const char *getDBPasswd        ();
	const char *getDBIP            ();
	int         getDBPort          () { return m_iDBPort; }

	//[RABBITMQ_INIT]
	void        nextRabbitmqInfo   (bool bEnforce=false);
	stRabbitmqInfo *getRabbitmqInfo();
	
	//[SERVER_INIT]                 
	const char *getServerName      ();
	const char *getServerIPAddr    ();
	int         getServerPort      () { return m_iServerPort;}
	int         getMaxCampaignCount() { return m_iMaxCampaignCount; }
	const char *getServerLogPath   ();
	const char *getServerCharSet   ();

	//[DKIM_INIT]
	vector<stDKIMInfo> *getDKIMList() { return &m_vecDKIMInfo; }

	//[MAIL_CONF_INIT]
	int         getMailRetryCount  () { return m_iMailRetryCount; }
	int         getMailRetryPeriod () { return m_iMailRetryPeriod; }
	
	//
	int         getTaxGroupNo      () { return m_iTaxGroupNo; }
	
	void        printFileName      ();
	
	static CEMSConfig * getInstance();

private:
	static CEMSConfig *m_pCEMSConfig;

	//[INIT]
	int    m_iDNSRefreshTimeInterval;
	int    m_iMAXMXRetryCount;
	char   m_chDNSFileName    [SZ_NAME+1];
	//[DBINFO]                
	char   m_chDBName         [SZ_NAME+1];
	char   m_chDBUser         [SZ_NAME+1];
	char   m_chDBPasswd       [SZ_PASSWD+1];
	char   m_chDBIP           [SZ_IP4ADDRESS+1];
	int    m_iDBPort;         
	//[RABBITMQ_INIT]
	int                    m_iRabbitmqInfoPos;
	time_t                 m_tModTime;
	stRabbitmqInfo        *m_pRabbitmqInfo;
	vector<stRabbitmqInfo> m_vecRabbitmqInfo;
	//[SERVER_INIT]
	char   m_chServerIP       [SZ_IP4ADDRESS+1];
	char   m_chServerName     [SZ_NAME+1];
	int    m_iServerPort;
	int    m_iMaxCampaignCount;
	char   m_chServerLogPath  [SZ_NAME+1];
	char   m_chServerCharSet  [SZ_NAME+1];
	//[DKIM_INIT]
	vector<stDKIMInfo> m_vecDKIMInfo;
	//[MAIL_CONF_INIT]
	int    m_iMailRetryCount;
	int    m_iMailRetryPeriod;
	
	//[GROUP_INIT]
	int    m_iTaxGroupNo;

};

#define theEMSConfig() CEMSConfig::getInstance()

#endif // __CEMS_CONFIG_HEADER__
