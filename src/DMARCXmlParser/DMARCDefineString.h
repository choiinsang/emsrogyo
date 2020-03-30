#ifndef __CDMARC_DEFINE_STRING_HEADER__
#define __CDMARC_DEFINE_STRING_HEADER__


#define START_TRANSACTION       "START TRANSACTION"
#define CALL_COMMIT             "COMMIT"
#define CALL_ROLLBACK           "ROLLBACK"

//############################################ LENGTH ############################################
                            
#define LEN_CpIdxKey        32
#define LEN_Domain          255 //80 //63 + 뒷자리
#define LEN_Title           255 //100
#define LEN_Name            100 //50
#define LEN_Id              255 //50
#define LEN_Email           255 //100
#define LEN_Date            30
#define LEN_Ip              16
#define LEN_Explain         200
#define LEN_ExplainLong     500


//############################################ QUERY ############################################

#define QueryInsert_info      "INSERT INTO report_info VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %s) "
#define QueryUpdate_info      "UPDATE report_info SET report_email='%s', report_org_name='%s', report_date_begin='%s', report_date_end='%s', policy_adkim='%s', policy_aspf='%s', policy_p='%s', policy_sp='%s', policy_pct=%s WHERE report_id='%s' AND policy_domain='%s' "
#define QueryInsert_record    "INSERT INTO report_record VALUES ('%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s') "
#define QueryUpdate_record    "UPDATE report_record SET count=%d, identifiers_header_from='%s', policy_evaluated_disposition='%s', policy_evaluated_dkim='%s', policy_evaluated_spf='%s', auth_results_spf_domain='%s', auth_results_spf_result='%s', auth_results_dkim_domain='%s', auth_results_dkim_result='%s' WHERE report_id='%s' AND source_ip='%s' "


//#define QuerySel_Campain                  "SELECT * FROM Campain WHERE DateSend <= now() and CpStep=-1"
#define QuerySel_Campain                  "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep FROM Campain WHERE DateSend <= now() AND CpStep=-1"
#define QueryUdt_Campain_CpStepInit       "UPDATE Campain SET CpStep=-1 WHERE CpStep=0 OR CpStep=1" //0:읽기시작 1:읽기완료 2:보내기완료
#define QueryUdt_Campain_CpStep0          "UPDATE Campain SET CpStep=0 WHERE CpIdx='%s'"
#define QueryUdt_Campain_CpStep1          "UPDATE Campain SET CpStep=1 WHERE CpIdx='%s'"
#define QueryUdt_Campain_CpStepComplete   "UPDATE Campain SET CpStep=2,DateEnd=now() WHERE CpIdx='%s'"

//#define QuerySel_CpMail                   "SELECT * FROM CpMail WHERE CpIdx='@1' AND SmtpStep=-1 ORDER BY MailIdx LIMIT @2"
#define QuerySel_CpMail                   "SELECT CpIdx,MailIdx,ToName,ToId,ToDomain,TryCnt FROM CpMail WHERE CpIdx='@1' AND SmtpStep=-1 ORDER BY MailIdx LIMIT @2"
#define QueryUdt_CpMail_SmtpStepInit      "UPDATE CpMail SET SmtpStep=-1 WHERE (TryCnt<%d AND SmtpStep<>7) AND CpIdx='%s'"
#define QueryUdt_CpMail_SmtpStep0         "UPDATE CpMail SET SmtpStep=0 WHERE MailIdx=%s AND CpIdx='%s'"
#define QueryUdt_CpMail_SmtpStep0StEd     "UPDATE CpMail SET SmtpStep=0 WHERE (MailIdx>=%s and MailIdx<=%s) AND CpIdx='%s' AND TryCnt<%d"
#define QueryUdt_CpMail_SmtpStep21        "UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s AND CpIdx='%s'"
#define QueryUdt_CpMail_SmtpStep21Chk7    "UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s AND CpIdx='%s' AND SmtpCode<>7 "
#define QueryUdt_CpMail_SmtpStepComplete  "UPDATE CpMail SET DateSend=now(),SmtpStep=7,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s AND CpIdx='%s'"
#define QueryUdt_CpMail_SmtpQRfail        "UPDATE CpMail SET DateSend=now(),SmtpCode=801,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s' AND SmtpStep=7"
#define QueryUdt_CpMail_Smtp              "UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s AND CpIdx='%s'"
#define QueryUdt_CpMail_SmtpChk7          "UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s AND CpIdx='%s' and SmtpCode<>7" //add
#define QueryCnt_CpMail_UnComplete        "SELECT COUNT(*) FROM CpMail WHERE SmtpStep<>7 AND TryCnt<%d AND CpIdx='%s'"

#define QueryIns_CodeExp                  "INSERT INTO CodeExp VALUES ('@11',@1,@2,@3,@4,'@5','@6')"

//----------------------------
#define QrySel_Campain_Index_prev_G       "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() AND (CpStep > -1 AND CpStep < 7) AND CpIdx='%s' AND SendMode='G' "
#define QrySel_Campain_Index_prev         "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() AND (CpStep > -1 AND CpStep < 7) AND CpIdx='%s' "

#define QrySel_Campain_Index              "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() AND (CpStep=-1 or CpStep=-7) AND CpIdx='%s' "

//#if defined	USE_RESET
//#define QrySel_Campain    "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() and (CpStep=-1 or CpStep=-7)"

#define QrySel_Campain_old                "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() and (CpStep=-1 or CpStep=-7)"
#define QrySel_Campain_new_G              "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() and (CpStep=-1 or CpStep=-7) AND SendMode='G' "
#define QrySel_Campain_new_P              "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr,ResetCnt FROM Campain WHERE DateSend <= now() and (CpStep=-1 or CpStep=-7) AND SendMode='P' "
#define QrySel_CheckModeField             "SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='Campain' AND COLUMN_NAME='SendMode' "

//#elif defined USE_WSTR //#ifdef USE_WSTR
//#define QrySel_Campain    "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep,UseWstr FROM Campain WHERE DateSend <= now() and CpStep=-1"
//#else
//#define QrySel_Campain    "SELECT CpIdx,CpTitle,SenderName,SenderEmail,DateSend,MailTitle,MailBody,CpStep FROM Campain WHERE DateSend <= now() and CpStep=-1"
//#endif

//Previous Uncompleted Process List
#define QrySel_Campaign_PrevRunStep            "SELECT CpIdx FROM Campain WHERE CpIdx <> '' AND  CpStep IN (0, 1) "

//Complete Campaign Check
#define QrySel_Campaign_CpStep                 "SELECT CpStep   FROM Campain WHERE CpIdx='%s'"

#define QrySel_CpMail_UnCompletetSmtpStepCount "SELECT COUNT(*) FROM CpMail  WHERE SmtpStep <> 7 AND CpIdx='%s' AND TryCnt<%d"
//#define QrySel_CpMail_UnCompletetSmtpStepCount "SELECT COUNT(*) FROM CpMail a LEFT JOIN CodeExp b ON a.CpIdx = b.CpIdx AND a.MailIdx = b.MailIdx AND a.TryCnt = b.TryCnt WHERE a.SmtpStep IN( 0, -1 ) AND b.ResultCode IS NULL AND a.CpIdx = '%s' "

#define QrySel_CpMail_UnCompletetSmtpStep      "SELECT CpIdx FROM CpMail  WHERE SmtpStep <> 7 AND CpIdx IN( %s ) AND TryCnt<%d GROUP BY CpIdx "
#define QrySel_CpMail_LastUpdateTime           "SELECT DateSend FROM CpMail WHERE CpIdx='%s' ORDER BY DateSend DESC LIMIT 1 "

#define QryUdt_Campain_CpStepInit       "UPDATE Campain SET CpStep=-1 WHERE CpStep=0 or CpStep=1" //0:읽기시작 1:읽기완료 2:보내기완료
#define QryUdt_Campain_CpStep0          "UPDATE Campain SET CpStep=0,  DateSend=now() WHERE CpIdx='%s'"
#define QryUdt_Campain_CpStep1          "UPDATE Campain SET CpStep=1  WHERE CpIdx='%s'"
#define QryUdt_Campain_CpStepM8         "UPDATE Campain SET CpStep=-8, DateEnd=now() WHERE CpIdx='%s'"
#define QryUdt_Campain_CpStepComplete   "UPDATE Campain SET CpStep=2,  DateEnd=now() WHERE CpIdx='%s'"

#define QrySel_Campain_CpStepComplete   "SELECT cp.CpIdx, IFNULL(cm.cnt, 0) FROM Campain cp LEFT OUTER JOIN (SELECT cm1.CpIdx, COUNT(*) AS cnt FROM CpMail cm1, Campain cp1 WHERE cm1.SmtpStep <> 7 AND cm1.TryCnt < %d AND cp1.CpIdx=cm1.CpIdx AND cp1.SendMode='P' GROUP BY cm1.CpIdx ) AS cm ON cm.CpIdx=cp.CpIdx WHERE cp.SendMode='P' AND cp.CpIdx IN ( %s ) "
//Persional Mail Campaign Update CpStep1
#define QryUdt_Campain_CpStepCpIdx      "UPDATE Campain SET CpStep=%d WHERE CpIdx = '%s' " 
  
#define QrySel_CpMail                   "SELECT CpIdx,MailIdx,ToName,ToId,ToDomain,TryCnt FROM CpMail WHERE CpIdx='%s' and SmtpStep=-1 ORDER BY MailIdx limit %d"
#define QrySel_CpMailOrderId            "SELECT CpIdx,MailIdx,ToName,ToId,ToDomain,TryCnt FROM CpMail WHERE CpIdx='%s' and SmtpStep=-1 ORDER BY ToId "
#define QrySel_CpMailOrderId_limit      "SELECT CpIdx,MailIdx,ToName,ToId,ToDomain,TryCnt FROM CpMail WHERE CpIdx='%s' and SmtpStep=-1 ORDER BY ToId limit %d"
#define QrySel_CpMailWstr               "SELECT CpMail.CpIdx,CpMail.MailIdx,ToName,ToId,ToDomain,TryCnt,WideStr FROM CpMail LEFT JOIN CpWstr ON (CpMail.CpIdx = CpWstr.CpIdx) and (CpMail.MailIdx = CpWstr.MailIdx) WHERE CpMail.CpIdx='%s' and SmtpStep=-1 ORDER BY MailIdx limit %d"
#define QrySel_CpMailWstrOrderId        "SELECT CpMail.CpIdx,CpMail.MailIdx,ToName,ToId,ToDomain,TryCnt,WideStr FROM CpMail LEFT JOIN CpWstr ON (CpMail.CpIdx = CpWstr.CpIdx) and (CpMail.MailIdx = CpWstr.MailIdx) WHERE CpMail.CpIdx='%s' and SmtpStep=-1 ORDER BY ToId "
#define QrySel_CpMailWstrOrderId_limit  "SELECT CpMail.CpIdx,CpMail.MailIdx,ToName,ToId,ToDomain,TryCnt,WideStr FROM CpMail LEFT JOIN CpWstr ON (CpMail.CpIdx = CpWstr.CpIdx) and (CpMail.MailIdx = CpWstr.MailIdx) WHERE CpMail.CpIdx='%s' and SmtpStep=-1 ORDER BY ToId limit %d"
#define QryUdt_CpMail_SmtpStepInit      "UPDATE CpMail SET SmtpStep=-1 WHERE (TryCnt<%d and SmtpStep<>7) and CpIdx='%s'"

#define QryUdt_CpMail_SmtpStep0         "UPDATE CpMail SET SmtpStep=0 WHERE MailIdx=%s and CpIdx='%s' and SmtpStep=-1 " //9add
#define QryUdt_CpMail_SmtpStep0StEd     "UPDATE CpMail SET SmtpStep=0 WHERE (MailIdx>=%s and MailIdx<=%d and SmtpStep=-1) and CpIdx='%s' and TryCnt<%d" //9add
#define QryUdt_CpMail_SmtpStep21        "UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s'"
#define QryUdt_CpMail_SmtpStep21Chk7    "UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s' and SmtpCode<>7"
#define QryUdt_CpMail_SmtpStepComplete  "UPDATE CpMail SET DateSend=now(),SmtpStep=7,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s'"
#define QryUdt_CpMail_SmtpQRfail        "UPDATE CpMail SET DateSend=now(),SmtpCode=801,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s' and SmtpStep=7"
#define QryUdt_CpMail_Smtp              "UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s'"
#define QryUdt_CpMail_SmtpChk7          "UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%s and CpIdx='%s' and SmtpCode<>7" //add

#define QryCnt_CpMail_UnComplete        "SELECT COUNT(*) FROM CpMail WHERE SmtpStep<>7 and TryCnt<%d and CpIdx='%s'"

#define QryIns_CodeExp                 "INSERT INTO CodeExp VALUES ('%s',%s,%d,%d,%d,'%s',NULL) ON DUPLICATE KEY UPDATE CpIdx='%s',MailIdx=%s,TryCnt=%d"
#define QryIns_CodeExpResult           "INSERT INTO CodeExp VALUES ('%s',%s,%d,%d,%d,'%s',%d) ON DUPLICATE KEY UPDATE CpIdx='%s',MailIdx=%s,TryCnt=%d"
//----------------------------

enum StrQuerySet{
	  iQryIns_Start=0
	, iQueryUdt_Campain_CpStepInit       //1)"UPDATE Campain SET CpStep=-1 WHERE CpStep=0 or CpStep=1" //0:읽기시작 1:읽기완료 2:보내기완료
	, iQueryUdt_Campain_CpStep0          //2)"UPDATE Campain SET CpStep=0 WHERE CpIdx='@1'"
	, iQueryUdt_Campain_CpStep1          //3)"UPDATE Campain SET CpStep=1 WHERE CpIdx='@1'"
	, iQueryUdt_Campain_CpStepComplete   //4)"UPDATE Campain SET CpStep=2,DateEnd=now() WHERE CpIdx='@1'"

	, iQueryUdt_CpMail_SmtpStepInit      //5)"UPDATE CpMail SET SmtpStep=-1 WHERE (TryCnt<@1 and SmtpStep<>7) and CpIdx='@2'"
	, iQueryUdt_CpMail_SmtpStep0         //6)"UPDATE CpMail SET SmtpStep=0 WHERE MailIdx=@1 and CpIdx='@11'"
	, iQueryUdt_CpMail_SmtpStep0StEd     //7)"UPDATE CpMail SET SmtpStep=0 WHERE (MailIdx>=@1 and MailIdx<=@2) and CpIdx='@11' and TryCnt<@3"
	, iQueryUdt_CpMail_SmtpStep21        //8)"UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=@2,TryCnt=@3 WHERE MailIdx=@1 and CpIdx='@11'"
	, iQueryUdt_CpMail_SmtpStep21Chk7    //9)"UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=@2,TryCnt=@3 WHERE MailIdx=@1 and CpIdx='@11' and SmtpCode<>7"
	, iQueryUdt_CpMail_SmtpStepComplete  //10)"UPDATE CpMail SET DateSend=now(),SmtpStep=7,SmtpCode=@3,TryCnt=@4 WHERE MailIdx=@1 and CpIdx='@11'"
	, iQueryUdt_CpMail_SmtpQRfail        //11)"UPDATE CpMail SET DateSend=now(),SmtpCode=801,TryCnt=@4 WHERE MailIdx=@1 and CpIdx='@11' and SmtpStep=7"
	, iQueryUdt_CpMail_Smtp              //12)"UPDATE CpMail SET DateSend=now(),SmtpStep=@2,SmtpCode=@3,TryCnt=@4 WHERE MailIdx=@1 and CpIdx='@11'"
	, iQueryUdt_CpMail_SmtpChk7          //13)"UPDATE CpMail SET DateSend=now(),SmtpStep=@2,SmtpCode=@3,TryCnt=@4 WHERE MailIdx=@1 and CpIdx='@11' and SmtpCode<>7" //add

	, iQryUdt_Campain_CpStepInit         //14)"UPDATE Campain SET CpStep=-1 WHERE CpStep=0 or CpStep=1" //0:읽기시작 1:읽기완료 2:보내기완료
	, iQryUdt_Campain_CpStep0            //15)"UPDATE Campain SET CpStep=0  WHERE CpIdx='%s'"
	, iQryUdt_Campain_CpStep1            //16)"UPDATE Campain SET CpStep=1  WHERE CpIdx='%s'"
	, iQryUdt_Campain_CpStepM8           //17)"UPDATE Campain SET CpStep=-8 WHERE CpIdx='%s'"
	, iQryUdt_Campain_CpStepComplete     //18)"UPDATE Campain SET CpStep=2,DateEnd=now() WHERE CpIdx='%s'"

	, iQryUdt_CpMail_SmtpStep0           //19)"UPDATE CpMail SET SmtpStep=0 WHERE MailIdx=%d and CpIdx='%s' and SmtpStep=-1 " //9add
	, iQryUdt_CpMail_SmtpStep0StEd       //20)"UPDATE CpMail SET SmtpStep=0 WHERE (MailIdx>=%d and MailIdx<=%d and SmtpStep=-1) and CpIdx='%s' and TryCnt<%d" //9add
	, iQryUdt_CpMail_SmtpStep21          //21)"UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s'"
	, iQryUdt_CpMail_SmtpStep21Chk7      //22)"UPDATE CpMail SET DateSend=now(),SmtpStep=21,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s' and SmtpCode<>7"
	, iQryUdt_CpMail_SmtpStepComplete    //23)"UPDATE CpMail SET DateSend=now(),SmtpStep=7,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s'"
	, iQryUdt_CpMail_SmtpQRfail          //24)"UPDATE CpMail SET DateSend=now(),SmtpCode=801,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s' and SmtpStep=7"
	, iQryUdt_CpMail_Smtp                //25)"UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s'"
	, iQryUdt_CpMail_SmtpChk7            //26)"UPDATE CpMail SET DateSend=now(),SmtpStep=%d,SmtpCode=%d,TryCnt=%d WHERE MailIdx=%d and CpIdx='%s' and SmtpCode<>7" //add

	, iQryIns_CodeExp                    //27)"INSERT INTO CodeExp VALUES ('%s',%d,%d,%d,%d,'%s',NULL) ON DUPLICATE KEY UPDATE CpIdx='%s',MailIdx=%d,TryCnt=%d"
	, iQryIns_CodeExpResult              //28)"INSERT INTO CodeExp VALUES ('%s',%d,%d,%d,%d,'%s',%d) ON DUPLICATE KEY UPDATE CpIdx='%s',MailIdx=%d,TryCnt=%d"
	, iQryIns_End
};

//----------------------------
//#define Query_UseStl
#define Qry_Strlen       400
#define Qry_StrlenBig    1000

#define CPSTEPM8_SendStopOrClose    -8
#define CPSTEPM7_ReqSendStopOrClose -7
#define CPSTEPB_DbBeforeRunning     -1
#define CPSTEP0_DbReadRunning       0
#define CPSTEP1_DbReadComplete      1
#define CPSTEP2_SendComplete        2

//########################################### STEP21 ###########################################
#define STEP21                      21

//Connect관련 에러
#define STEP21_C101_ConErr          101
#define STEP21_C101Exp_ConErr       "Connect Error(Direct)"
#define STEP21_C102_AddCltErr       102
#define STEP21_C102Exp_AddCltErr    "Connect Add Client Failed"
#define STEP21_C110_ConErr          110
#define STEP21_C110Exp_ConErr       "Connect Error"

#define STEP21_C151_ConErr          151
#define STEP21_C151Exp_ConErr       "Connect A Error(Direct)"
#define STEP21_C152_AddCltErr       152
#define STEP21_C152Exp_AddCltErr    "Connect A Add Client Failed"
#define STEP21_C160_ConErr          160
#define STEP21_C160Exp_ConErr       "Connect A Error"
#define STEP21_C161_ConErr          161
#define STEP21_C161Exp_ConErr       "Connect A Multi Error" //A레코드 다수 존재

//Mx관련 에러                       
#define STEP21_C201_CreatMxErr      201
#define STEP21_C201Exp_CreatMxErr   "Create Mx Error"

//DisConnect관련 에러               
#define STEP21_C301_DisconErr       301
#define STEP21_C301Exp_DisconErr    "DisConnect Error"

#define STEP21_C351_DisconErr       351
#define STEP21_C351Exp_DisconErr    "DisConnect A Error"

//Unknown관련 에러                  
#define STEP21_C401_UnknownStr      401
#define STEP21_C401Exp_UnknownStr   "Unknown String" //실제문자열로 대체

//Packet크기관련 에러
#define STEP21_C501_PacketSmall     501
#define STEP21_C501Exp_PacketSmall  "Packet size too small" //실제문자열로 대체
//##############################################################################################


#define SIZE_EncodeBuf        300000
#define SIZE_AddBuf           1200
#define SIZE_HeaderBuf        4096
#define SIZE_ThreadBuf        SIZE_EncodeBuf + SIZE_AddBuf

//단일 Define
#define NOT_NUMBER            -1


#define SplitContent_StartNum 90
#define SplitItemCnt_Max      90
#define SplitInfoCnt_Max      SplitItemCnt_Max*2 + 1

#define SplitReserve_NULL      0
#define SplitReserve_Name     -1
#define SplitReserve_Email    -2
#define SplitReserve_CpIdx    -3 
#define SplitReserve_MailIdx  -4 
#define SplitReserve_ToId     -5 
#define SplitReserve_ToDomain -6 


#define CRLF                      "\r\n"
#define CRLFx2                    "\r\n\r\n"
#define CRLFx2dotCRLF             "\r\n\r\n.\r\n"

#define ENHEAD_HELO               "HELO"
#define ENHEAD_RCPT_TO            "RCPT TO"
#define ENHEAD_MAIL_FROM          "MAIL FROM"
#define ENHEAD_DATA               "DATA"
#define ENHEAD_SUBJECT            "Subject"
#define ENHEAD_MESSAGE_ID         "Message-ID"
#define ENHEAD_DATE               "Date"
#define ENHEAD_FROM               "From"
#define ENHEAD_TO                 "To"
#define ENHEAD_X_Mailer           "X-Mailer: Gabia Web-Mailer v1.2 - gabia.com"
#define ENHEAD_MIME_VERSION       "MIME-Version: 1.0"
////#define ENHEAD_CType          "Content-Type:text/html;\r\n	charset=\"ks_c_5601-1987\"\r\nContent-Transfer-Encoding: base64\r\n\r\n"

#define CONTENT_TYPE              "Content-Type"
#define CONTENT_TYPE_TXT_HTML     "text/html"
#define CONTENT_TYPE_MULTI_MIXED  "multipart/mixed"
#define CONTENT_TYPE_MULTI_ALTER  "multipart/alternative"

#define CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding: base64"
#define ENHEAD_REPLY_TO           "Reply-To"  
#define ENHEAD_QUIT               "QUIT"

#define CONTENT_CHARSET           "charset"
#define CONTENT_CHARSET_UTF8      "UTF-8"
#define CONTENT_CHARSET_EUC_KR    "EUC-KR"
#define CONTENT_CHARSET_KS_C      "ks_c_5601-1987"



#define WIDESTR_ITER	0x02 //테스트할땐'^'사용
//#define WIDESTR_ITER	'^'

//---------------------------------------- DefineString ----------------------------------------
#define MAXLENGTH_DOMAIN 256
//----------------------------------------------------------------------------------------------

#define SPLIT_ByBody		0 //mUseWstr
#define SPLIT_ByTitle		1 //mUseWstrTT
#define SPLIT_END       2 //


#endif  //__CDMARC_DEFINE_STRING_HEADER__
