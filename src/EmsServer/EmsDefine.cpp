#include "EmsDefine.h"
#include "EmsDefineString.h"

const char * getHeaderStringFromKey(int hNumber){
	switch(hNumber){
		case MAILINFO_QINDEX              :  return MHEADER_QINDEX      ; //0
		case MAILINFO_CPNUMBER            :  return MHEADER_CPNUMBER    ; //1
		case MAILINFO_CPMAILKEY           :  return MHEADER_CPMAILKEY   ; //2
		case MAILINFO_MAILNUMBER          :  return MHEADER_MAILNUMBER  ; //3 
		case MAILINFO_MAILINDEX           :  return MHEADER_MAILINDEX   ; //4
		case MAILINFO_TONAME              :  return MHEADER_TONAME      ; //5 
		case MAILINFO_TOID                :  return MHEADER_TOID        ; //6 
		case MAILINFO_TODOMAIN            :  return MHEADER_TODOMAIN    ; //7 
		case MAILINFO_WIDESTR             :  return MHEADER_WIDESTR     ; //8 
		case MAILINFO_MAILTITLE           :  return MHEADER_MAILTITLE   ; //9 
		case MAILINFO_SENDERNAME          :  return MHEADER_SENDERNAME  ; //10 
		case MAILINFO_SENDEREMAIL         :  return MHEADER_SENDEREMAIL ; // 
		case MAILINFO_SENDERDOMAIN        :  return MHEADER_SENDERDOMAIN; // 
		case MAILINFO_USEWSTR             :  return MHEADER_USEWSTR     ; //
		case MAILINFO_SMTPCODE            :  return MHEADER_SMTPCODE    ; //
		case MAILINFO_SMTPSTEP            :  return MHEADER_SMTPSTEP    ; //
		case MAILINFO_TRYCOUNT            :  return MHEADER_TRYCOUNT    ; //
		case MAILINFO_DBNAME              :  return MHEADER_DBNAME      ; //
		case MAILINFO_GROUPNUMBER         :  return MHEADER_GROUPNUMBER ; //
		case MAILINFO_GROUPNAME           :  return MHEADER_GROUPNAME   ; //
		case MAILINFO_TR_TYPE             :  return MHEADER_TR_TYPE     ; //
			
		default : return (const char *)NULL;
	}
};


const char * getMsgHeaderStringFromKey(int hNumber){
	switch(hNumber){
		case MSGINFO_TYPE                 : return MSGHEADER_TYPE;
		case MSGINFO_STATE                : return MSGHEADER_STATE;
		case MSGINFO_CPNUMBER             : return MSGHEADER_CPNUMBER;
		case MSGINFO_CPMAILKEY            : return MSGHEADER_CPMAILKEY;
		case MSGINFO_MAILNUMBER           : return MSGHEADER_MAILNUMBER;
		case MSGINFO_MAILINDEX            : return MSGHEADER_MAILINDEX;
		case MSGINFO_TONAME               : return MSGHEADER_TONAME;
		case MSGINFO_TOID                 : return MSGHEADER_TOID;
		case MSGINFO_TODOMAIN             : return MSGHEADER_TODOMAIN;
		case MSGINFO_WIDESTR              : return MSGHEADER_WIDESTR;
		case MSGINFO_MAILTITLE            : return MSGHEADER_MAILTITLE;
		case MSGINFO_SENDERNAME           : return MSGHEADER_SENDERNAME;
		case MSGINFO_SENDEREMAIL          : return MSGHEADER_SENDEREMAIL;
		case MSGINFO_SENDERDOMAIN         : return MSGHEADER_SENDERDOMAIN;
		case MSGINFO_USEWSTR              : return MSGHEADER_USEWSTR;
		case MSGINFO_SMTPCODE             : return MSGHEADER_SMTPCODE;
		case MSGINFO_SMTPSTEP             : return MSGHEADER_SMTPSTEP;
		case MSGINFO_TRYCOUNT             : return MSGHEADER_TRYCOUNT;
		case MSGINFO_ERR_STR              : return MSGHEADER_ERR_STR;
		case MSGINFO_ERR_CODE             : return MSGHEADER_ERR_CODE;
		case MSGINFO_TIMESTAMP            : return MSGHEADER_TIMESTAMP;
		case MSGINFO_SERVER_NUMBER        : return MSGHEADER_SERVER_NUMBER;
		case MSGINFO_SERVER_NAME          : return MSGHEADER_SERVER_NAME;
		case MSGINFO_SERVER_IP            : return MSGHEADER_SERVER_IP;
		case MSGINFO_GETMSG_TYPE          : return MSGHEADER_GETMSG_TYPE;
		case MSGINFO_GROUPNUMBER          : return MSGHEADER_GROUPNUMBER;
		case MSGINFO_GROUPNAME            : return MSGHEADER_GROUPNAME;
		case MSGINFO_DBQUERY_SET          : return MSGHEADER_DBQUERY_SET; 
		case MSGINFO_TR_TYPE              : return MSGHEADER_TR_TYPE;
			
		case MSGINFO_END:	                    //MSGINFO END
		default : return (const char *)NULL;
	}
};


const char * getQryStringFromKey(int hNumber, int iGroupNo, int iTaxGroupNo){
	switch(hNumber){
		case iQryUdt_Campaign_CpStepInit     : return QryUdt_Campaign_CpStepInit     ;
		case iQryUdt_Campaign_CpStep0        : return QryUdt_Campaign_CpStep0        ;
		case iQryUdt_Campaign_CpStep1        : return QryUdt_Campaign_CpStep1        ;
		case iQryUdt_Campaign_CpStepComplete : return QryUdt_Campaign_CpStepComplete ;

		case iQryUdt_Mail_SmtpStep0          : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpStep0       :QryUdt_Mail_SmtpStep0       ;
		case iQryUdt_Mail_SmtpStep0StEd      : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpStep0StEd   :QryUdt_Mail_SmtpStep0StEd   ;
		case iQryUdt_Mail_SmtpStep21         : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpStep21      :QryUdt_Mail_SmtpStep21      ;
		case iQryUdt_Mail_SmtpStep21Chk7     : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpStep21Chk7  :QryUdt_Mail_SmtpStep21Chk7  ;
		case iQryUdt_Mail_SmtpStepComplete   : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpStepComplete:QryUdt_Mail_SmtpStepComplete;
		case iQryUdt_Mail_SmtpQRfail         : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpQRfail      :QryUdt_Mail_SmtpQRfail      ;
		case iQryUdt_Mail_Smtp               : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_Smtp            :QryUdt_Mail_Smtp            ;
		case iQryUdt_Mail_SmtpChk7           : return (iGroupNo==iTaxGroupNo)? QryUdt_Tax_Mail_SmtpChk7        :QryUdt_Mail_SmtpChk7        ;

		case iQryIns_CodeExp                 : return QryIns_CodeExp                 ;
		case iQryIns_CodeExpResult           : return QryIns_CodeExpResult           ;

		default : return (const char *)NULL; 
	}
};
