#ifndef __CEMS_DEFINE_INFORMATION_HEADER__
#define __CEMS_DEFINE_INFORMATION_HEADER__

#ifndef NULL    
#define NULL    0
#endif
//----------------------------------------
// INITIONALIZE DEFAULT CONFIGURATION INFORMATION
//----------------------------------------
const char IEMS_SERVER_INI   []        = "/etc/IEmsServer.ini";
const char IEMS_AGENT_INI    []        = "/etc/IEmsAgent.ini";

const char IEMS_VERSION      []        = "2.0.13";
const char IEMS_SUB_VERSION  []        = "QR1";
const char IEMS_RELEASE_DATE []        = "20181218";

const char DEFAULT_DOMAIN_GABIA_COM[]  = "gabia.com";
const char DEFAULT_NO_IPADDRESS    []  = "0.0.0.0";

const int INIT_DEFAULT_DNSREFRESHTIME  = 86400;
const int INIT_DEFAULT_MAXRETRYCOUNT   = 3;
const int INIT_DEFAULT_DBPORT          = 3306;
const int INIT_DEFAULT_RMQPORT         = 5672;
const int INIT_DEFAULT_SERVERPORT      = 0;
const int INIT_DEFAULT_CAMPAIGNCOUNT   = 200;
const int INIT_DEFAULT_MAILRETYRCOUNT  = 3;         //Send Retry Count
const int INIT_DEFAULT_MAILRETYRPERIOD = 300;       //Retry Wait Time(sec)

//DKIM
const int  INIT_DEFAULT_DKIMSIGNALG    = 1;         //DKIM_SIGN_RSASHA256	1	/* an RSA-signed SHA256 digest */
const	char INIT_DEFAULT_DKIMHCANON []  = "relaxed"; //DKIM Header Canonicalization
const	char INIT_DEFAULT_DKIMBCANON []  = "relaxed"; //DKIM Body Canonicalization
const int  DEFAULT_BODY_LEN            = 128;

//----------------------------------------
// EMAIL SETTING VALUES
//----------------------------------------
const int MAIL_PORT                    = 25;

//----------------------------------------
// SET THREAD VALUES
//----------------------------------------
const int MAX_WORKERTHREAD_COUNT       = 7;
const int MAX_TOTAL_MSG_COUNT          = 20000; //메시지 최대 수신 카운트
const int MAX_MX_HOST_COUNT            = 10;
const int MAX_GROUP_PROC_COUNT         = 30;
const int MAX_CONNECTION_COUNT         = 200; 
const int MAX_ENQUEUE_SIZE             = 200; 
const int MAX_ENQUEUE_SIZE_I           = 200; 
const int MAX_CAMPAIGNLIST_COUNT       = 20;   //CampaignList 체크시 한 번에 체크하는 수
const int MAX_DB_CHECK_WAIT_COUNT      = 5;    // 15
const int MAX_DB_WAIT_TIME             = 3600; // 1hour
const int MAX_CPLIST_CHECK_WAIT_TIME   = 60;   // 60sec
const int MAX_GROUP_CHECK_WAIT_TIME    = 600;  // 600sec
const int REQ_CONFIG_SEND_PERIOD       = 10;   // Config 정보 요청을 보내는 주기
const int DEFAULT_COMPLETECHECKTIME    = 2;    // 메일 전송 상태 확인 주기
const int DEFAULT_INTERVAL_MSEC        = 700;  // 발송서버 기본 메일 전송 간격msec

const int THREAD_SLEEPTIME_1MSEC       = 1000;
const int THREAD_SLEEPTIME_5MSEC       = 5000;
const int THREAD_SLEEPTIME_10MSEC      = 10000;
const int THREAD_SLEEPTIME_50MSEC      = 50000;
const int THREAD_SLEEPTIME_100MSEC     = 100000;
const int THREAD_SLEEPTIME_500MSEC     = 500000;
const int THREAD_SLEEPTIME_1SEC        = 1000000;
const int THREAD_RMQ_WAIT_TIME         = 5;


//----------------------------------------
// ENUM SET
//----------------------------------------
enum _THREADSTATE{
	 TSTATE_PREREADY     = 0
	,TSTATE_READY        = 1
	,TSTATE_RUN          = 2
	,TSTATE_PAUSE        = 3
	,TSTATE_STOP         = 4
};

enum _CLINETSTATE{
	 CLIENT_START        = 0
	,CLIENT_CONNECT_FAIL = 1
	,CLIENT_SENDBUF_FAIL = 2
	,CLIENT_DONE         = 3
};

//----------------------------------------
// GROUP DEFINITION
//----------------------------------------
enum _GROUP_DEFINITION{
	  I_ALL_GROUP    = -1
	, I_NONE_GROUP   = 0
	//Group Info Set
	, I_GROUP_TAX    = 3
};

const char RMQ_EXCHANGE_CMD        []  = "CMD";
const char RMQ_ROUTE_KEY_CMD       []  = "CMD";
const char RMQ_EXCHANGE_EA_CMD     []  = "EACMD";
const char RMQ_ROUTE_KEY_EA_CMD    []  = "EACMD";
                                   
const char RMQ_EXCHANGE_MAIL       []  = "MAIL";
const char RMQ_ROUTE_KEY_MAIL      []  = "MAIL";   //[Not Use]
const char RMQ_EXCHANGE_LOG        []  = "EMSLOG";
const char RMQ_ROUTE_KEY_LOG       []  = "EMSLOG"; //[Not Use]
const char RMQ_EXCHANGE_COMMON     []  = "COMMON";
const char RMQ_ROUTE_KEY_COMMON    []  = "COMMON";

const char RMQ_ROUTE_KEY_NONE_GROUP[]  = "NONEGROUP";

const unsigned int SZ_CMD_HEADER_T     = 15;
                                      
const char CMD_DELIMITER               = '|';
const char CMD_DELIMITER_END           = '@';

const char CMD_YES                 []  = "Y";
const char CMD_NO                  []  = "N";

const char NONE_GROUP              []  = "NONE_GROUP";
const char ALL_GROUP               []  = "ALL_GROUP";
const char ALL_SERVER              []  = "ALL_SERVER";
const char NULLSTRING              []  = "NULL";
const char NULLDOMAINNAME          []  = "NULLDOMAINNAME";

//CMD MESSAGE TYPES
const char CMD_TYPE                []  = "CMDTYPE";
const char CMD_INDEX_KEY           []  = "CMDINDEXKEY";
const char CMD_SERVER_NUMBER       []  = "CMDSERVERNUMBER";
const char CMD_SERVER_NAME         []  = "CMDSERVERNAME";
const char CMD_SERVER_MULTI_GROUP  []  = "CMDSERVERMULTIGROUP";
const char CMD_SERVER_IPADDR       []  = "CMDSERVERIPADDR";
const char CMD_SERVER_GROUPNUMBER  []  = "CMDSERVERGROUPNUMBER";
const char CMD_SERVER_GROUPNAME    []  = "CMDSERVERGROUPNAME";
const char CMD_SERVER_MSGQ         []  = "CMDSERVERMSGQ";
const char CMD_SERVER_CMDQ         []  = "CMDSERVERCMDQ";
const char CMD_SERVER_COMMONQ      []  = "CMDSERVERCOMMONQ";
const char CMD_SERVER_STATE        []  = "CMDSERVERSTATE";

//CMD_SERVER_STATE
const char SERVER_STATE_ENABLE     []  = "enable";
const char SERVER_STATE_DISABLE    []  = "disable";

#define CMD_TYPE_CREATE_QUEUE         "CREATE" 
#define CMD_TYPE_INIT_CAMPAIGN        "CPINIT" 
#define CMD_TYPE_DELETE_QUEUE         "DELETE" 
#define CMD_TYPE_PAUSE_QUEUE          "PAUSE"  
#define CMD_TYPE_RESTART_QUEUE        "RESTRAT"
#define CMD_TYPE_STOP_QUEUE           "STOP" 
#define CMD_TYPE_STOP_SERVER          "STOPSERVER" 
#define CMD_TYPE_CHK_PROCESSLIST      "PROCLIST"
#define CMD_TYPE_SET_SERVER_CONF      "SERVERCONF"
#define CMD_TYPE_CHK_CONN_HB          "CONNECTIONHB"

const char CMD_TYPE_LIST[][SZ_CMD_HEADER_T+1] = {
	 CMD_TYPE_CREATE_QUEUE 
	,CMD_TYPE_INIT_CAMPAIGN
	,CMD_TYPE_DELETE_QUEUE 
	,CMD_TYPE_PAUSE_QUEUE  
	,CMD_TYPE_RESTART_QUEUE
	,CMD_TYPE_STOP_QUEUE
	,CMD_TYPE_STOP_SERVER
	,CMD_TYPE_CHK_PROCESSLIST
	,CMD_TYPE_SET_SERVER_CONF
	,CMD_TYPE_CHK_CONN_HB
};

enum CMD_TYPE_INFO {
	 CMD_TYPE_INFO_CREATE_QUEUE  = 0
	,CMD_TYPE_INFO_INIT_CAMPAIGN
	,CMD_TYPE_INFO_DELETE_QUEUE 
	,CMD_TYPE_INFO_PAUSE_QUEUE  
	,CMD_TYPE_INFO_RESTART_QUEUE
	,CMD_TYPE_INFO_STOP_QUEUE
	,CMD_TYPE_INFO_STOP_SERVER
	,CMD_TYPE_INFO_CHK_PROCESSLIST
	,CMD_TYPE_INFO_SET_SERVER_CONF
	,CMD_TYPE_INFO_CHK_CONN_HB
	,CMD_TYPE_INFO_END
};

//----------------------------------------
// PROCESS MODE ( Group or Personal Mode)
//----------------------------------------
const char STR_MODE_I[] = "I";
const char STR_MODE_M[] = "M";

enum PROCESS_MODE {
	  MODE_NONE = -1
	, MODE_I    =  0
	, MODE_M    =  1
};

//----------------------------------------
// SET DEFAULT SIZE VALUES
//----------------------------------------
const unsigned int SZ_INDEX_KEY              = 32;
const unsigned int SZ_TINYBUFFER             = 8;
const unsigned int SZ_INT_NUMBER             = 11;
const unsigned int SZ_LONG_NUMBER            = 20;
const unsigned int SZ_TMPBUFFER              = 512;
const unsigned int SZ_MAXBUFFER              = 102400;

const unsigned int SZ_NAME                   = 255;
const unsigned int SZ_PASSWD                 = 255;                                          
const unsigned int SZ_IP4ADDRESS             = 15;
const unsigned int SZ_DEFAULTBUFFER          = 4096;
                                          
const unsigned int SZ_MAX_SENDBUFFER         = 102400;
const unsigned int SZ_MAX_RECVBUFFER         = 102400;
const unsigned int SZ_MAX_CLIENT             = 5000 ;
const unsigned int SZ_MAX_TIMERVALUE         = 3000;

//----------------------------------------
// SET MAIL DEFINE / LOG INFO VALUE Share
// getHeaderStringFromKey(int hNumber) <= use
//----------------------------------------
const char MHEADER_QINDEX             [] = "qindex";             //0
const char MHEADER_CPNUMBER           [] = "campaign_no";        //1
const char MHEADER_CPMAILKEY          [] = "campaign_mailkey";   //2
const char MHEADER_MAILNUMBER         [] = "mail_no";            //3
const char MHEADER_MAILINDEX          [] = "mailidx";            //4
const char MHEADER_TONAME             [] = "to_name";            //5
const char MHEADER_TOID               [] = "to_id";              //6
const char MHEADER_TODOMAIN           [] = "to_domain";          //7
const char MHEADER_WIDESTR            [] = "wide_str";           //8
const char MHEADER_MAILTITLE          [] = "mail_title";         //9
const char MHEADER_SENDERNAME         [] = "sender_name";        //10
const char MHEADER_SENDEREMAIL        [] = "sender_email";       //11
const char MHEADER_SENDERDOMAIN       [] = "sender_domain";      //12
const char MHEADER_USEWSTR            [] = "use_wstr";           //13
const char MHEADER_SMTPCODE           [] = "smtp_code";          //14
const char MHEADER_SMTPSTEP           [] = "smtp_step";          //15
const char MHEADER_TRYCOUNT           [] = "try_cnt";            //16
const char MHEADER_DBNAME             [] = "db_name";            //17
const char MHEADER_GROUPNUMBER        [] = "group_no";           //18
const char MHEADER_GROUPNAME          [] = "group_name";         //19
const char MHEADER_TR_TYPE            [] = "tr_type";            //20

const char MAIL_MSG_BODY              [] = "msg_body"; 

enum CP_MAILINFO{
	  MAILINFO_QINDEX = 0               //"qindex";
	, MAILINFO_CPNUMBER                 //"campaign_no";
	, MAILINFO_CPMAILKEY                //"campaign_mailkey";
	, MAILINFO_MAILNUMBER               //"mail_no";
	, MAILINFO_MAILINDEX                //"mailidx";
	, MAILINFO_TONAME                   //"to_name";
	, MAILINFO_TOID                     //"to_id";
	, MAILINFO_TODOMAIN                 //"to_domain";
	, MAILINFO_WIDESTR                  //"wide_str";
	, MAILINFO_MAILTITLE                //"mail_title";
	, MAILINFO_SENDERNAME               //"sender_name";
	, MAILINFO_SENDEREMAIL              //"sender_email";
	, MAILINFO_SENDERDOMAIN             //"sender_domain";
	, MAILINFO_USEWSTR                  //"use_wstr";
	, MAILINFO_SMTPCODE                 //"smtp_code";
	, MAILINFO_SMTPSTEP                 //"smtp_step";
	, MAILINFO_TRYCOUNT                 //"try_cnt";
	, MAILINFO_DBNAME                   //"db_name";
	, MAILINFO_GROUPNUMBER              //"group_no";
	, MAILINFO_GROUPNAME                //"group_name";
	, MAILINFO_TR_TYPE                  //"tr_type";
	
	, CP_MAILINFO_END                   //MAILINFO END
};

const char * getHeaderStringFromKey(int hNumber);

//----------------------------------------
// SET LOG DEFINE
//----------------------------------------
//LOG PROCESS OPTIONS
const int  ILOG_OPT_PROC   = 1;
const int  ILOG_OPT_SENDER = 2;

//----------------------------------------
// SET MAIL DEFINE / LOG INFO VALUE Share
// MESSAGE HEADER TYPES
//----------------------------------------
const char MSGHEADER_TYPE                [] = "msg_type";            //0 
const char MSGHEADER_STATE               [] = "msg_state";           //1 
const char MSGHEADER_CPNUMBER            [] = "campaign_no";         //2
const char MSGHEADER_CPMAILKEY           [] = "campaign_mailkey";    //3
const char MSGHEADER_MAILNUMBER          [] = "mail_no";             //4
const char MSGHEADER_MAILINDEX           [] = "mailidx";             //5
const char MSGHEADER_TONAME              [] = "to_name";             //6
const char MSGHEADER_TOID                [] = "to_id";               //7
const char MSGHEADER_TODOMAIN            [] = "to_domain";           //8
const char MSGHEADER_WIDESTR             [] = "wide_str";            //9
const char MSGHEADER_MAILTITLE           [] = "mail_title";          //10
const char MSGHEADER_SENDERNAME          [] = "sender_name";         //11
const char MSGHEADER_SENDEREMAIL         [] = "sender_email";        //12
const char MSGHEADER_SENDERDOMAIN        [] = "sender_domain";       //13
const char MSGHEADER_USEWSTR             [] = "use_wstr";            //14
const char MSGHEADER_SMTPCODE            [] = "smtp_code";           //15
const char MSGHEADER_SMTPSTEP            [] = "smtp_step";           //16
const char MSGHEADER_TRYCOUNT            [] = "try_cnt";             //17
const char MSGHEADER_ERR_STR             [] = "explain";             //18
const char MSGHEADER_ERR_CODE            [] = "result_code";         //19
const char MSGHEADER_TIMESTAMP           [] = "msg_timestamp";       //20
const char MSGHEADER_SERVER_NUMBER       [] = "server_number";       //21
const char MSGHEADER_SERVER_NAME         [] = "server_name";         //22
const char MSGHEADER_SERVER_IP           [] = "server_ipaddr";       //23
const char MSGHEADER_GETMSG_TYPE         [] = "msg_getmsg_type";     //24
const char MSGHEADER_GROUPNUMBER         [] = "group_no";            //25
const char MSGHEADER_GROUPNAME           [] = "group_name";          //26
const char MSGHEADER_DBQUERY_SET         [] = "db_query_set";        //27
const char MSGHEADER_TR_TYPE             [] = "tr_type";             //28

const char MSGHEADER_MSG_BODY            [] = "msg_body";

enum MSGINFO{
	  MSGINFO_TYPE = 0               //"msg_type"
	, MSGINFO_STATE                  //"msg_state"
	, MSGINFO_CPNUMBER               //"campaign_no"
	, MSGINFO_CPMAILKEY              //"campaign_mailkey"
	, MSGINFO_MAILNUMBER             //"mail_no"
	, MSGINFO_MAILINDEX              //"mailidx"
	, MSGINFO_TONAME                 //"cpToName"
	, MSGINFO_TOID                   //"cpToID"
	, MSGINFO_TODOMAIN               //"cpToDomain"
	, MSGINFO_WIDESTR                //"cpWideString"
	, MSGINFO_MAILTITLE              //"cpTitle"
	, MSGINFO_SENDERNAME             //"cpSenderName"
	, MSGINFO_SENDEREMAIL            //"cpSenderEmail"
	, MSGINFO_SENDERDOMAIN           //"cpSenderDomain"
	, MSGINFO_USEWSTR                //"cpUseWstr"
	, MSGINFO_SMTPCODE               //"cpSmtp_code"
	, MSGINFO_SMTPSTEP               //"cpSmtp_step"
	, MSGINFO_TRYCOUNT               //"cpTry_count"
	, MSGINFO_ERR_STR                //"explain"
	, MSGINFO_ERR_CODE               //"result_code"
	, MSGINFO_TIMESTAMP              //"msg_timestamp"
	, MSGINFO_SERVER_NUMBER          //"server_number"
	, MSGINFO_SERVER_NAME            //"server_name"
	, MSGINFO_SERVER_IP              //"server_ipaddr"
	, MSGINFO_GETMSG_TYPE            //"msg_getmsg_type"
	, MSGINFO_GROUPNUMBER            //"group_no"
	, MSGINFO_GROUPNAME              //"group_name"
	, MSGINFO_DBQUERY_SET            //"db_query_set"
	, MSGINFO_TR_TYPE                //"tr_type"
	
	, MSGINFO_END                    //MSGINFO END
};

const char * getMsgHeaderStringFromKey(int hNumber);

enum MSG_TYPE {
	  MSG_TYPE_DEBUG  = 0            // 0
	, MSG_TYPE_CONNECT               // 1
	, MSG_TYPE_HEARTBEAT             // 2
	, MSG_TYPE_STATE                 // 3
	, MSG_TYPE_DBUPDATE              // 4
	, MSG_TYPE_DBINSERT              // 5
	, MSG_TYPE_REQ_GET               // 6
};


//MSGINFO_STATE 에 전달되는 값
enum GETMSG_TYPE {
	  NULL_TYPE           = -1 // Type이 필요 없는 경우
	, GET_SERVER_CONF     =  0 // 서버 정보를 전달하고 해당 서버의 설정정보를 요청
	, GET_PROC_LIST       =  1 // 처리중인 PROCESS LIST를 요청
};

#define STRLEN_CodeResult  50
#define LEN_Explain        200
#define LEN_ExplainLong    500
#define NOT_NUMBER        -1

//----------------------------------------
// SET LOG DEFINE
//----------------------------------------
#define LOG_NONE           0x00  //[0000] No Log Write
#define LOG_WQ             0x01  //[0001] Write log to Queue, and send log to (Rabbitmq)queue
#define LOG_WF             0x10  //[0010] Write log file 
#define LOG_WA             0x11  //[0011] Write log both file and queue 

#endif   //__CEMS_DEFINE_INFORMATION_HEADER__

            
