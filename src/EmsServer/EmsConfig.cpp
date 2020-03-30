#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "EmsConfig.h"
#include "EmsDefineString.h"


CEMSConfig::CEMSConfig()
: m_iDNSRefreshTimeInterval(0)
,	m_iMAXMXRetryCount       (0)
,	m_iDBPort                (0)
, m_iRabbitmqInfoPos       (-1)
, m_pRabbitmqInfo          (NULL)
{
	//[INIT]
	memset(m_chDNSFileName, '\0', (SZ_NAME+1));
	//[DBINFO]
	memset(m_chDBName,   '\0', (SZ_NAME+1));
	memset(m_chDBUser,   '\0', (SZ_NAME+1));
	memset(m_chDBPasswd, '\0', (SZ_PASSWD+1));
	memset(m_chDBIP,     '\0', (SZ_IP4ADDRESS+1));
	//[RABBITMQ_INIT]
	time(&m_tModTime);
	//[SERVER_INIT]
	memset(m_chServerIP,        '\0', (SZ_IP4ADDRESS+1));
	memset(m_chServerLogPath,   '\0', (SZ_NAME+1));
	memset(m_chServerCharSet,   '\0', (SZ_NAME+1));
}

CEMSConfig::~CEMSConfig()
{
}


CEMSConfig *CEMSConfig::m_pCEMSConfig = NULL;

//-----------------------------------
// Get Config Singleton Pointer
CEMSConfig * CEMSConfig::getInstance()
{
	if ( m_pCEMSConfig == NULL )
	{
		m_pCEMSConfig = new CEMSConfig();
	}
	return m_pCEMSConfig;
}

//-----------------------------------
// Init Config with INI FileName('/etc/IEms*.ini)
bool CEMSConfig::InitConfig(const char * l_pFileName)
{
	CIniFile INIFile;
	bool     retval = false;
	
	if(l_pFileName == NULL){
		return retval;
	}
	else{
		if( (retval = INIFile.InitINIFile(l_pFileName)) ==  false){
			//init CIniFile failed
			//Print Log <= ischoi 
			return retval;
		}
		else{
			char INIT[][50] = {
				  "INIT"              //[0]
				, "DNSLIST_FILENAME"  //[1]
				, "DNSREFRESHTIME"    //[2]
				, "MAX_MXRETRYCOUNT"  //[3]
			};

			//[DBINFO]
			char DBINFO[][30] = {
				  "DBINFO"  //[0]
				, "DBNAME"  //[1]
				, "DBIP"    //[2]
				, "DBUSER"  //[3]
				, "DBPASS"  //[4]  
				, "DBPORT"  //[5]
			};
			//[RABBITMQ_INIT]			
			char RABBITMQ_INIT[][50] = {
				  "RABBITMQ_INIT"   //[0]
				, "RABBITMQ_IP"     //[1]
				, "RABBITMQ_PORT"   //[2]
				, "RABBITMQ_USER"   //[3]
				, "RABBITMQ_PASSWD" //[4]
				, "RABBITMQ_VHOST"  //[5]
			};

			//[SERVER_INIT]
			char SERVER_INIT[][50] = {
				  "SERVER_INIT"              //[0]
				, "SERVER_IP"                //[1]
				, "SERVER_NAME"              //[2]
				, "SERVER_PORT"              //[3]
				, "SERVER_MAX_CAMPAIGNCOUNT" //[4]
				, "SERVER_LOG_PATH"          //[5]
				, "SERVER_CHAR_SET"          //[6]
				, "SERVER_TAX_GROUP_NO"      //[7]
			};
			
			//[DKIM_INIT]
			char DKIM_INIT[][50] = {
				  "DKIM_INIT"
				, "DKIM_SIGN_ALG"
				, "DKIM_KEY_PATH"
				, "DKIM_SELECTOR"
				, "DKIM_DOMAIN"
				, "DKIM_HEADERCANON"
				, "DKIM_BODYCANON"
			};
			
			//[MAIL_CONF_INIT]
			char MAIL_CONF_INIT[][50]= {
				  "MAIL_CONF_INIT"
				, "MAIL_RETRY_COUNT"
				, "MAIL_RETRY_PERIOD"
			};
			

			char NULLFILENAME[4]  = {""};
			char LOCALHOST   [16] = {"127.0.0.1"};
			char USER        [10] = {"guest"};
			char PASSWD      [10] = {"guest"};
			char VHOST       [4]  = {"/"};
				
			//[INIT]
			m_iDNSRefreshTimeInterval = 0;
			m_iMAXMXRetryCount = 0;
			memset(m_chDNSFileName, '\0', SZ_NAME+1);

			//[DBINFO]
			memset(m_chDBName,   '\0', SZ_NAME+1);
			memset(m_chDBUser,   '\0', SZ_NAME+1);
			memset(m_chDBPasswd, '\0', SZ_PASSWD+1);
			memset(m_chDBIP,     '\0', SZ_IP4ADDRESS+1);

			//[RABBITMQ_INIT]


			{	//[INIT]
				INIFile.GetProfileString(INIT[0], INIT[1], NULLFILENAME, m_chDNSFileName);
				m_iDNSRefreshTimeInterval = INIFile.GetProfileInt(INIT[0], INIT[2], INIT_DEFAULT_DNSREFRESHTIME);
				m_iMAXMXRetryCount        = INIFile.GetProfileInt(INIT[0], INIT[3], INIT_DEFAULT_MAXRETRYCOUNT);
			}

			{//[DBINFO]
				INIFile.GetProfileString(DBINFO[0], DBINFO[1], NULLFILENAME, m_chDBName);
				INIFile.GetProfileString(DBINFO[0], DBINFO[2], LOCALHOST,    m_chDBIP);
				INIFile.GetProfileString(DBINFO[0], DBINFO[3], NULLFILENAME, m_chDBUser);
				INIFile.GetProfileString(DBINFO[0], DBINFO[4], NULLFILENAME, m_chDBPasswd);
				m_iDBPort  = INIFile.GetProfileInt(DBINFO[0], DBINFO[5], INIT_DEFAULT_DBPORT);
			}
			
			{//[RABBITMQ_INIT]
				stRabbitmqInfo tmpRabbitmqInfo;
				memset(&tmpRabbitmqInfo, 0, sizeof(tmpRabbitmqInfo));
				char tmpstr[SZ_NAME+1];
				int retval = I_SUCCESS;
				
				for(int i=1; ;i++){
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", RABBITMQ_INIT[1], i);
					retval = INIFile.GetProfileString(RABBITMQ_INIT[0], tmpstr, LOCALHOST, tmpRabbitmqInfo._chRabbitmq_HostName);
					if(retval < I_SUCCESS)
						break;
					else{
						struct hostent * ht = gethostbyname(tmpRabbitmqInfo._chRabbitmq_HostName);
						if(ht != NULL){
							snprintf(tmpRabbitmqInfo._chRabbitmq_IP, sizeof(tmpRabbitmqInfo._chRabbitmq_IP), "%s", inet_ntoa( *(struct in_addr*)ht->h_addr_list[0])); 
						}
						else{
							break;
						}
					}

					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", RABBITMQ_INIT[2], i);
					tmpRabbitmqInfo._iRabbitmq_Port = INIFile.GetProfileInt(RABBITMQ_INIT[0], tmpstr, INIT_DEFAULT_RMQPORT);
	
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", RABBITMQ_INIT[3], i);
					retval = INIFile.GetProfileString(RABBITMQ_INIT[0], tmpstr, USER, tmpRabbitmqInfo._chRabbitmq_User);
					if(retval < I_SUCCESS)
						break;
	
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", RABBITMQ_INIT[4], i);
					retval = INIFile.GetProfileString(RABBITMQ_INIT[0], tmpstr, PASSWD, tmpRabbitmqInfo._chRabbitmq_Passwd);
					if(retval < I_SUCCESS)
						break;
	
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", RABBITMQ_INIT[5], i);
					retval = INIFile.GetProfileString(RABBITMQ_INIT[0], tmpstr, VHOST, tmpRabbitmqInfo._chRabbitmq_Vhost);
					if(retval < I_SUCCESS)
						break;
	
					printf("[RABBITMQ_INI_CONFIG] [RMQINFO][%s][%s][%d][%s][%s][%s]\n"
		           , tmpRabbitmqInfo._chRabbitmq_HostName
		           , tmpRabbitmqInfo._chRabbitmq_IP
		           , tmpRabbitmqInfo._iRabbitmq_Port
		           , tmpRabbitmqInfo._chRabbitmq_User
		           , tmpRabbitmqInfo._chRabbitmq_Passwd
		           , tmpRabbitmqInfo._chRabbitmq_Vhost );
		           
		           
					m_vecRabbitmqInfo.push_back(tmpRabbitmqInfo);
				}
				nextRabbitmqInfo(true);
			}

			{//[SERVER_INIT]
				INIFile.GetProfileString(SERVER_INIT[0], SERVER_INIT[1], LOCALHOST,    m_chServerIP);
				INIFile.GetProfileString(SERVER_INIT[0], SERVER_INIT[2], NULLFILENAME, m_chServerName);
				m_iServerPort       = INIFile.GetProfileInt(SERVER_INIT[0], SERVER_INIT[3], INIT_DEFAULT_SERVERPORT);
				m_iMaxCampaignCount = INIFile.GetProfileInt(SERVER_INIT[0], SERVER_INIT[4], INIT_DEFAULT_CAMPAIGNCOUNT);
				INIFile.GetProfileString(SERVER_INIT[0], SERVER_INIT[5], NULLFILENAME,         m_chServerLogPath);
				INIFile.GetProfileString(SERVER_INIT[0], SERVER_INIT[6], CONTENT_CHARSET_UTF8, m_chServerCharSet);
				m_iTaxGroupNo       = INIFile.GetProfileInt(SERVER_INIT[0], SERVER_INIT[7], I_GROUP_TAX);
			}
			
			{//[DKIM_INIT]
				stDKIMInfo tmpDKIMInfo;
				memset(&tmpDKIMInfo, 0, sizeof(tmpDKIMInfo));
				char tmpstr[SZ_NAME+1];
				int retval = I_SUCCESS;
				
				for(int i=1; ;i++){
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[1], i);
					tmpDKIMInfo._iDKIMSignAlg = INIFile.GetProfileInt(DKIM_INIT[0], tmpstr, INIT_DEFAULT_DKIMSIGNALG);
					
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[2], i);
					retval = INIFile.GetProfileString(DKIM_INIT[0], tmpstr, NULLFILENAME, tmpDKIMInfo._chDKIMKeyPath);
					if(retval < I_SUCCESS)
						break;

					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[3], i);
					retval = INIFile.GetProfileString(DKIM_INIT[0], tmpstr, NULLFILENAME, tmpDKIMInfo._chDKIMSelector);
					if(retval < I_SUCCESS)
						break;
					
					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[4], i);
					retval = INIFile.GetProfileString(DKIM_INIT[0], tmpstr, NULLFILENAME, tmpDKIMInfo._chDKIMDomainName);
					if(retval < I_SUCCESS)
						break;

					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[5], i);
					retval = INIFile.GetProfileString(DKIM_INIT[0], tmpstr, const_cast<char*>(INIT_DEFAULT_DKIMHCANON), tmpDKIMInfo._chDKIMHeaderCanon);

					memset(tmpstr, 0, SZ_NAME+1);
					sprintf(tmpstr, "%s_%d", DKIM_INIT[6], i);
					retval = INIFile.GetProfileString(DKIM_INIT[0], tmpstr, const_cast<char*>(INIT_DEFAULT_DKIMBCANON), tmpDKIMInfo._chDKIMBodyCanon);
					if(retval < I_SUCCESS)
						break;

					m_vecDKIMInfo.push_back(tmpDKIMInfo);					
				}
			}
			
			{//[MAIL_CONF_INIT]
				m_iMailRetryCount  = INIFile.GetProfileInt(MAIL_CONF_INIT[0], MAIL_CONF_INIT[1], INIT_DEFAULT_MAILRETYRCOUNT);
				m_iMailRetryPeriod = INIFile.GetProfileInt(MAIL_CONF_INIT[0], MAIL_CONF_INIT[2], INIT_DEFAULT_MAILRETYRPERIOD);
	
				INIFile.EndINIFile();
				retval = true;
			}
		}
	}
	
	return retval;
}

//[Function](Check String)
//======================================
const char * CEMSConfig::checkString(char * pStr)
{
	if(strlen(pStr) == 0)
		return (const char*)NULL;
	else 
		return (const char*)pStr;	
}

//[INIT]
//======================================

const char *CEMSConfig::getDNSFileName()
{
	return checkString(m_chDNSFileName);
}

//[DBINFO]
//======================================
const char *CEMSConfig::getDBName()
{
	return checkString(m_chDBName);
}

const char *CEMSConfig::getDBUser()
{
	return checkString(m_chDBUser);
}

const char *CEMSConfig::getDBPasswd()
{
	return checkString(m_chDBPasswd);
}

const char *CEMSConfig::getDBIP()
{
	return checkString(m_chDBIP);
}

//[RABBITMQ_INIT]
//======================================
void CEMSConfig::nextRabbitmqInfo(bool bEnforce)
{
	//printf("[%s][%s][%d][POS:%d]", __FILE__, __FUNCTION__, __LINE__, m_iRabbitmqInfoPos);
	time_t currtime;
	time(&currtime);
	
	if(bEnforce == false){
		if( difftime(currtime, m_tModTime) < THREAD_RMQ_WAIT_TIME)
			return ;
	}

	time(&m_tModTime);
	if(m_vecRabbitmqInfo.size() == 0){
		m_iRabbitmqInfoPos = -1;
	}
	else{
		if((m_iRabbitmqInfoPos == -1)
			||(m_iRabbitmqInfoPos+1 >= m_vecRabbitmqInfo.size())){
			m_iRabbitmqInfoPos = 0;
		}
		else{
			m_iRabbitmqInfoPos++;
		}
	}
}

stRabbitmqInfo *CEMSConfig::getRabbitmqInfo()
{
	if(m_iRabbitmqInfoPos == -1)
		return (stRabbitmqInfo *)NULL;
	else{
		vector<stRabbitmqInfo>::iterator itr = m_vecRabbitmqInfo.begin();
		return  (stRabbitmqInfo*)&(*(itr+m_iRabbitmqInfoPos));
	}
}


//[SERVER_INIT]
//======================================
const char * CEMSConfig::getServerIPAddr()
{
	return checkString(m_chServerIP);
}

const char * CEMSConfig::getServerName()
{
	return checkString(m_chServerName);
}

const char * CEMSConfig::getServerLogPath()
{
	return checkString(m_chServerLogPath);
}

const char * CEMSConfig::getServerCharSet()
{
	return checkString(m_chServerCharSet);
}

//-----------------------------------
// Print Config Values
void CEMSConfig::printFileName(){
	printf("DNS FileName : %s\n", getDNSFileName());	
}

