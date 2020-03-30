#ifndef __CEMS_DEFINE_STRING_HEADER__
#define __CEMS_DEFINE_STRING_HEADER__


//Define Table Max Column Count
//----------------------------------------
#define SMTP_STEP_InitBefore        -1
#define SMTP_STEP00_Init             0
#define SMTP_STEP01_ConnectEND       1
#define SMTP_STEP02_HeloEND          2
#define SMTP_STEP03_MailFromEND      3
#define SMTP_STEP04_RcptToEND        4
#define SMTP_STEP05_DataEND          5
#define SMTP_STEP06_DataContentEND   6
#define SMTP_STEP07_QuitOrRsetEnd    7

#define SMTP_ErrorOccur             -1
#define SMTP_SkipOccur              -2

// Ems DB Table Data Info
//----------------------------------------
#define LEN_Campaign_no              20  //bigint length
#define LEN_CpMailKey                32
#define LEN_Domain                   255 //80 //63 + 뒷자리
#define LEN_Title                    255 //100
#define LEN_Name                     100 //50
#define LEN_Id                       255 //50
#define LEN_Email                    255 //100
#define LEN_Date                     30
#define LEN_Ip                       16
#define LEN_Explain                  200
#define LEN_ExplainLong              500

// Check Database
#define QrySel_CheckModeField             "SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='campaign' AND COLUMN_NAME='group_no' OR COLUMN_NAME='tr_type' "

//Select Limit Count
#define MaxReadCount_Campaign             20
#define MaxReadCount_CpMail               20
#define FldCnt_Campaign_Info              13

#define QrySel_Campaign_Info              "SELECT no,mailkey,sender_name,sender_email,mail_title,mail_body,use_wstr,reset_cnt,db_name,group_no,tr_type,step,del_mail_body FROM campaign WHERE step=%d AND tr_type='%s' AND reg_date <= now() "
#define QrySel_Campaign_Info_No           "SELECT no,mailkey,sender_name,sender_email,mail_title,mail_body,use_wstr,reset_cnt,db_name,group_no,tr_type,step,del_mail_body FROM campaign WHERE no=%s AND tr_type='%s' "
#define ADD_CHECK_NO                      "AND no>%s "
#define ADD_LIMIT                         "LIMIT %d "

//Pause Ems Mail Thread (Mode_M)
#define FldCnt_Campaign_Pause             4
#define QrySel_Campaign_Pause             "SELECT no,db_name,group_no,tr_type FROM campaign WHERE step=-7 AND reg_date <= now() "

//----------------------------
//Previous Uncompleted Process List
#define QrySel_Campaign_no_PrevRunStep    "SELECT no FROM campaign WHERE no <> '' AND  step IN (0, 1)  AND tr_type = '%s' "

#define QrySel_Campaign_no_prev           "SELECT no,mailkey,sender_name,sender_email,mail_title,mail_body,use_wstr,reset_cnt,db_name,group_no,tr_type,step,del_mail_body FROM campaign WHERE no=%s AND reg_date <= now() AND (step > -1 AND step < 2) AND tr_type='%s' "
#define QrySel_Campaign_no_Ready          "SELECT no,mailkey,sender_name,sender_email,mail_title,mail_body,use_wstr,reset_cnt,db_name,group_no,tr_type,step,del_mail_body FROM campaign WHERE no=%s AND reg_date <= now() AND (step = -1 OR step = -7) "


//Complete Campaign Check   //step 0:읽기진행 1:읽기완료 2:보내기완료
#define QrySel_Mail_UnComplete            "SELECT COUNT(*) FROM %s.mails WHERE campaign_no=%s AND smtp_step>=-1 AND smtp_step<>7 AND try_cnt<%d "

#define QryUdt_Campaign_CpStepInit        "UPDATE campaign SET step=-1 WHERE step=0 OR step=1"

//UPDATE Campaign
#define QryUdt_Campaign_CpStepM8          "UPDATE campaign SET step=-8, end_date=NOW() WHERE no=%s "
#define QryUdt_Campaign_CpStepM3          "UPDATE campaign SET step=-3, end_date=NOW() WHERE no=%s "
#define QryUdt_Campaign_CpStep0           "UPDATE campaign SET step=0, send_date=NOW() WHERE no=%s "
#define QryUdt_Campaign_CpStep1           "UPDATE campaign SET step=1  WHERE no=%s "
#define QryUdt_Campaign_CpStepComplete    "UPDATE campaign SET step=2, end_date=NOW() WHERE no=%s "
#define QryUdt_Campaign_CpStepComplete_Del_mailbody "UPDATE campaign SET mail_body=NULL, step=2, end_date=NOW() WHERE no=%s AND del_mail_body='Y' "


       
//나누어서 처리하자.

#define FldCnt_Mail_StepComplete         2
#define QrySel_Mail_StepComplete         "SELECT campaign_no, COUNT(*) AS cnt FROM %s.mails WHERE smtp_step>=-1 AND smtp_step <> 7 AND try_cnt < %d AND campaign_no IN (%s) GROUP BY campaign_no "

#define FldCnt_Mail_Info                 9
#define QrySel_Mail_Info                 "SELECT no,mailidx,campaign_no,to_name,to_id,to_domain,try_cnt,email,wide_str FROM %s.mails WHERE campaign_no=%s AND smtp_step=-1 "


#define QryUdt_Mail_SmtpStepInit         "UPDATE %s.mails SET smtp_step=-1 WHERE (try_cnt<%s AND smtp_step>=-1 AND smtp_step<>7) AND campaign_no=%s"
#define QryUdt_Mail_SmtpStepM2           "UPDATE %s.mails SET smtp_step=-2 WHERE no=%s"
#define QryUdt_Mail_SmtpStepM1           "UPDATE %s.mails SET smtp_step=-1, send_date='0000-00-00 00:00:00' WHERE no IN ( %s )  "
#define QryUdt_Mail_SmtpStep0            "UPDATE %s.mails SET smtp_step=0 WHERE no=%s AND smtp_step=-1 "
#define QryUdt_Mail_SmtpStep0StEd        "UPDATE %s.mails SET smtp_step=0 WHERE (no>=%s AND no<=%s AND smtp_step=-1) AND campaign_no=%s AND try_cnt<%s "
#define QryUdt_Mail_SmtpStep21           "UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Mail_SmtpStep21Chk7       "UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 "
#define QryUdt_Mail_SmtpStepComplete     "UPDATE %s.mails SET send_date=NOW(),smtp_step=7, smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Mail_SmtpQRfail           "UPDATE %s.mails SET send_date=NOW(),smtp_code=801,try_cnt=%s WHERE no=%s AND smtp_step=7 "
#define QryUdt_Mail_Smtp                 "UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Mail_SmtpChk7             "UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 " //add

#define QryIns_CodeExp                   "INSERT INTO %s.mails_detail (`mail_no`,`try_cnt`,`smtp_step`,`smtp_code`,`explain`) VALUES (%s,%s,%s,%s,'%s') ON DUPLICATE KEY UPDATE `smtp_step`=%s,`smtp_code`=%s,`explain`='%s' "
#define QryIns_CodeExpResult             "INSERT INTO %s.mails_detail (`mail_no`,`try_cnt`,`smtp_step`,`smtp_code`,`explain`,`result_code`) VALUES (%s,%s,%s,%s,'%s',%s) ON DUPLICATE KEY UPDATE `smtp_step`=%s,`smtp_code`=%s,`explain`='%s',`result_code`=%s "

// For 세금계산서
#define QryUdt_Tax_Mail_SmtpStepInit     "UPDATE %s.mails SET ApplyTaxdb='N',smtp_step=-1 WHERE (try_cnt<%s AND smtp_step>=-1 AND smtp_step<>7) AND campaign_no=%s"
#define QryUdt_Tax_Mail_SmtpStep0        "UPDATE %s.mails SET ApplyTaxdb='N',smtp_step=0 WHERE no=%s AND smtp_step=-1 "
#define QryUdt_Tax_Mail_SmtpStep0StEd    "UPDATE %s.mails SET ApplyTaxdb='N',smtp_step=0 WHERE (no>=%s AND no<=%s AND smtp_step=-1) AND campaign_no=%s AND try_cnt<%s "
#define QryUdt_Tax_Mail_SmtpStep21       "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Tax_Mail_SmtpStep21Chk7   "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 "
#define QryUdt_Tax_Mail_SmtpStepComplete "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_step=7, smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Tax_Mail_SmtpQRfail       "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_code=801,try_cnt=%s WHERE no=%s AND smtp_step=7 "
#define QryUdt_Tax_Mail_Smtp             "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s "
#define QryUdt_Tax_Mail_SmtpChk7         "UPDATE %s.mails SET ApplyTaxdb='N',send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 " //add


//----------------------------
// 'server_info' Table
//----------------------------
#define FldCnt_ServerConfigInfo           5
#define QrySel_ServerConfigInfo           "SELECT si.no,gi.no,gi.name,gi.q_name,si.usable FROM server_info si,group_info gi WHERE si.group_no=gi.no AND si.ip='%s' "
#define QrySel_ServerConfigInfo_GroupNoIP "SELECT si.no,gi.no,gi.name,gi.q_name,si.usable FROM server_info si,group_info gi WHERE si.group_no=gi.no AND gi.no=%s AND si.ip='%s' "

//----------------------------
// 'group_info' Table
//----------------------------
#define FldCnt_GroupInfo                3
#define QrySel_GroupInfo                "SELECT no,name,q_name FROM group_info WHERE no=%d "
#define QrySel_GroupInfoALL             "SELECT no,name,q_name FROM group_info WHERE no<>'' "

//----------------------------
// Server HeartBeat
//----------------------------
#define QryUdt_ServerHB                 "UPDATE server_info SET heartbeat=NOW() WHERE no<>0 AND group_no=%s AND ip='%s' "
//----------------------------


enum StrQuerySet{
	  iQryIns_Start=0

	, iQryUdt_Campaign_CpStepInit        //"UPDATE campaign SET step=-1 WHERE step=0 OR step=1" //0:읽기시작 1:읽기완료 2:보내기완료
	, iQryUdt_Campaign_CpStep0           //"UPDATE campaign SET step=0, send_Date=NOW() WHERE no=%s " 
	, iQryUdt_Campaign_CpStep1           //"UPDATE campaign SET step=1  WHERE no=%s "                 
	, iQryUdt_Campaign_CpStepComplete    //"UPDATE campaign SET step=2, end_Date=NOW() WHERE no=%s "  

	, iQryUdt_Mail_SmtpStep0             //"UPDATE %s.mails SET smtp_step=0 WHERE campaign_no=%s AND mailidx='%s' AND smtp_step=-1 "
	, iQryUdt_Mail_SmtpStep0StEd         //"UPDATE %s.mails SET smtp_step=0 WHERE (no>=%s AND no<=%s AND smtp_step=-1) AND campaign_no=%s AND try_cnt<%s "
	, iQryUdt_Mail_SmtpStep21            //"UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE campaign_no=%s AND mailidx=%s "
	, iQryUdt_Mail_SmtpStep21Chk7        //"UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE campaign_no=%s AND mailidx=%s AND smtp_code<>7 "
	, iQryUdt_Mail_SmtpStepComplete      //"UPDATE %s.mails SET send_date=NOW(),smtp_step=7, smtp_code=%s,try_cnt=%s WHERE campaign_no=%s AND mailidx=%s "
	, iQryUdt_Mail_SmtpQRfail            //"UPDATE %s.mails SET send_date=NOW(),smtp_code=801,try_cnt=%s WHERE no=%s AND campaign_no=%s AND smtp_step=7 "
	, iQryUdt_Mail_Smtp                  //"UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s AND mailidx=%s AND campaign_no=%s "
	, iQryUdt_Mail_SmtpChk7              //"UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s AND mailidx=%s AND campaign_no=%s AND smtp_code<>7 "

	, iQryIns_CodeExp                    //"INSERT INTO %s.mails_detail ('mail_no','try_cnt','smtp_step','smtp_code','explain') VALUES (%s,%s,%s,%s,'%s') ON DUPLICATE KEY UPDATE mail_no=%s,try_cnt=%d "                  
	, iQryIns_CodeExpResult              //"INSERT INTO %s.mails_detail ('mail_no','try_cnt','smtp_step','smtp_code','explain','result_code') VALUES (%s,%s,%s,%s,'%s',%s) ON DUPLICATE KEY UPDATE mail_no=%s,try_cnt=%d " 
	, iQryIns_End
};

//Get DB Query From Query Number And Group Number
//----------------------------------------
const char * getQryStringFromKey(int hNumber, int iGroupNo, int iTaxGroupNo);

//#define Query_UseStl
//----------------------------------------
#define Qry_Strlen                    400
#define Qry_StrlenBig                 1000

#define CPSTEPM8_SendStopOrClose     -8
#define CPSTEPM7_ReqSendStopOrClose  -7
#define CPSTEPM3_DbException         -3
#define CPSTEPB_DbBeforeRunning      -1
#define CPSTEP0_DbReadRunning         0
#define CPSTEP1_DbReadComplete        1
#define CPSTEP2_SendComplete          2

//----------------------------------------
// SMTP Response Code
//----------------------------------------
#define RESP_220                      220
#define RESP_221                      221
#define RESP_354                      354
#define RESP_250                      250

//----------------------------------------
// Mail Error Return Code
//----------------------------------------
#define STEP21                        21
                                      
//Connect관련 에러                    
#define STEP21_C101_ConErr            101
#define STEP21_C101Exp_ConErr         "Connect Error(Direct)"
#define STEP21_C102_AddCltErr         102
#define STEP21_C102Exp_AddCltErr      "Connect Add Client Failed"
#define STEP21_C110_ConErr            110
#define STEP21_C110Exp_ConErr         "Connect Error"
                                      
#define STEP21_C151_ConErr            151
#define STEP21_C151Exp_ConErr         "Connect A Error(Direct)"
#define STEP21_C152_AddCltErr         152
#define STEP21_C152Exp_AddCltErr      "Connect A Add Client Failed"
#define STEP21_C160_ConErr            160
#define STEP21_C160Exp_ConErr         "Connect A Error"
#define STEP21_C161_ConErr            161
#define STEP21_C161Exp_ConErr         "Connect A Multi Error" //A레코드 다수 존재
                                      
//Mx관련 에러                         
#define STEP21_C201_CreatMxErr        201
#define STEP21_C201Exp_CreatMxErr     "Create Mx Error"
                                      
//DisConnect관련 에러                 
#define STEP21_C301_DisconErr         301
#define STEP21_C301Exp_DisconErr      "DisConnect Error"
                                      
#define STEP21_C351_DisconErr         351
#define STEP21_C351Exp_DisconErr      "DisConnect A Error"
                                      
//Unknown관련 에러                    
#define STEP21_C401_UnknownStr        401
#define STEP21_C401Exp_UnknownStr     "Unknown String" //실제문자열로 대체
                                      
//Packet크기관련 에러                 
#define STEP21_C501_PacketSmall       501
#define STEP21_C501Exp_PacketSmall    "Packet size too small" //실제문자열로 대체


// Mail Data Info
//----------------------------------------
#define SIZE_EncodeBuf                300000
#define SIZE_AddBuf                   1200
#define SIZE_HeaderBuf                4096
#define SIZE_ThreadBuf                SIZE_EncodeBuf + SIZE_AddBuf
                                      
#define NOT_NUMBER                   -1
                                      
#define SPLIT_ByBody                  0 
#define SPLIT_ByTitle                 1 
#define SPLIT_END                     2
                                      
#define SplitContent_StartNum         90
#define SplitItemCnt_Max              90
#define SplitInfoCnt_Max              SplitItemCnt_Max*2 + 1

#define SplitReserve_NULL             0
#define SplitReserve_Name            -1
#define SplitReserve_Email           -2
#define SplitReserve_CpIdx           -3
#define SplitReserve_CpMail_No       -4
#define SplitReserve_ToId            -5
#define SplitReserve_ToDomain        -6


// Mail Define String
//----------------------------------------
#define CRLF                         "\r\n"
#define CRLFx2                       "\r\n\r\n"
#define CRLFx2dotCRLF                "\r\n\r\n.\r\n"
                                     
#define ENHEAD_HELO                  "HELO"
#define ENHEAD_RCPT_TO               "RCPT TO"
#define ENHEAD_MAIL_FROM             "MAIL FROM"
#define ENHEAD_DATA                  "DATA"
#define ENHEAD_SUBJECT               "Subject"
#define ENHEAD_MESSAGE_ID            "Message-ID"
#define ENHEAD_DATE                  "Date"
#define ENHEAD_FROM                  "From"
#define ENHEAD_TO                    "To"
#define ENHEAD_X_Mailer              "X-Mailer: Gabia Web-Mailer v1.2 - gabia.com"
#define ENHEAD_MIME_VERSION          "MIME-Version: 1.0"
////#define ENHEAD_CType             "Content-Type:text/html;\r\n	charset=\"ks_c_5601-1987\"\r\nContent-Transfer-Encoding: base64\r\n\r\n"
                                     
#define CONTENT_TYPE                 "Content-Type"
#define CONTENT_TYPE_TXT_HTML        "text/html"
#define CONTENT_TYPE_MULTI_MIXED     "multipart/mixed"
#define CONTENT_TYPE_MULTI_ALTER     "multipart/alternative"
                                     
#define CONTENT_TRANSFER_ENCODING    "Content-Transfer-Encoding: base64"
#define ENHEAD_REPLY_TO              "Reply-To"  
#define ENHEAD_QUIT                  "QUIT"

#define CONTENT_CHARSET              "charset"
#define CONTENT_CHARSET_UTF8         "UTF-8"
#define CONTENT_CHARSET_EUC_KR       "EUC-KR"
#define CONTENT_CHARSET_KS_C         "ks_c_5601-1987"

// Wide String Delimiter
//----------------------------------------
#define WIDESTR_ITER	               0x02

#endif  //__CEMS_DEFINE_STRING_HEADER__
