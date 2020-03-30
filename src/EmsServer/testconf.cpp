#include <iostream>
#include <stdio.h>
#include <time.h>
#include "EmsCommon.h"
#include "EmsDKIM.h"
#include "EmsDefineString.h"
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include "EmsConfig.h"

using namespace boost;
using namespace std;

void checkFlag(DKIM_STAT dstat, const char * p){
	if(dstat != DKIM_STAT_OK){
		printf("error : %d[str:%s]\n", dstat, p);
		exit(0);
	}
}

void testThrow(bool bRet){
	if(bRet){
		return;
	}
	else 
		throw "test";
	return;
}

void checkmap(){ // test unordered_map
	boost::unordered_map<std::string, std::string> tmap;
	boost::unordered_map<std::string, std::string>::iterator itr_tmap;
	tmap.insert(std::make_pair<std::string, std::string>("t1", "a1")); 
	tmap.insert(std::make_pair<std::string, std::string>("t2", "a2")); 
	tmap.insert(std::make_pair<std::string, std::string>("t3", "a3")); 
	tmap.insert(std::make_pair<std::string, std::string>("t4", "a4")); 
	tmap.insert(std::make_pair<std::string, std::string>("t5", "a5")); 
	std::cout << " u_map Contain :" << std::endl;


	for(itr_tmap = tmap.begin(); itr_tmap != tmap.end(); itr_tmap++){
		std::cout << "-" << itr_tmap->first << "--"<<itr_tmap->second <<std::endl;
	}
	
	std::cout << " Bucket Contain :" << std::endl;
	for(int i = 0; i < tmap.bucket_count(); i++){
		for(itr_tmap = tmap.begin(); itr_tmap != tmap.end(); itr_tmap++){
			std::cout <<  " ^ " << itr_tmap->first << " ^ " << itr_tmap->second << endl;
		}
	}

}

template< typename T >
struct array_deleter
{
  void operator ()( T const * p)
  { 
    delete[] p; 
  }
};


#define SUBJECT "Subject :"

void inputPrint(const char* pBuf){
	printf("pBuf : %s\n", pBuf);
};

int main(int argc, char *argv[])
{
	CEMSConfig     * pEmsCfg     = theEMSConfig();
	if(pEmsCfg->InitConfig(EMS_AGENT_INI) == true)
		cout << "[EMS AGENT] Read Config Success...[" << EMS_AGENT_INI<<"] "  << endl;
	else 
		cout << "[EMS AGENT] Read Config Failed..." << endl;
	sleep(30);
//	char h_title[]="test header";
//	printf("%s", SUBJECT (const char*)h_title);
//	return 0;
//	
//	
//	bool bUseWstr = bool(atoi("1"));
//	printf("bUseWstr = bool(atoi('1')) [%s]\n", bUseWstr?"true":"false");
//	bUseWstr = bool(atoi("0"));
//	printf("bUseWstr = bool(atoi('0')) [%s]\n", bUseWstr?"true":"false");
//	bUseWstr = bool(atoi("-1"));
//	printf("bUseWstr = bool(atoi('-1')) [%s]\n", bUseWstr?"true":"false");
//	bUseWstr = -1;
//	printf("bUseWstr = bool(atoi('-1')) [%s]\n", bUseWstr?"true":"false");
//
//	shared_ptr<int> sp = shared_ptr<int>( new int[10], array_deleter<int>() );
//	if(sp.get() == NULL)
//		printf("OK\n");
//	else
//		printf("NO GOOD\n");
//	return 0;
//	
//	
//	for(int i=0; i<200; i++){
//		if((i == 27) ||((i-'0') == 27))
//			continue;
//		printf("[%d : %c] [%d : %c]\n", i, i, i-'0', i-'0');
//	}
//	return 0;
//
	char jobid     []=  "jobid";
	char keypath   []=  "/etc/postfix/mailselector.key";
	char selector  []=  "mailselector";
	char domainName[]=  "gcloud02.co.kr";

	dkim_alg_t   signalg  = DKIM_SIGN_RSASHA256;
	CEmsDKIM vdkim(keypath, selector, domainName, "relaxed", "simple", signalg);

	DKIM_STAT status;
	int icount = 0;
	time_t startt, endt;
	time(&startt);

		char	HEADER01[]=	"Received: received data 0";
		char	HEADER02[]=	"Received: received data 1";


		
//		char	HEADER03[]=	"Received: received data 2";
		char	HEADER03[]=	"Content-Type: text/html;\r\ncharset=\"utf-8\"";

		char	HEADER04[]=	"Received: received data 3 part 1\r\n\t data 3 part 2";
		char	HEADER05[]=	"From: Murray S. Kucherawy <msk@sendmail.com>";
		char	HEADER06[]=	"To: Sendmail Test Address <sa-test@sendmail.net>";
		char	HEADER07[]=	"Date: Thu, 05 May 2005 11:59:09 -0700";
		char	HEADER08[]=	"Subject: DKIM test message";
		char	HEADER09[]=	"Message-ID: <439094BF.5010709@sendmail.com>";
		char	HEADER10[]=	"Cc: user@example.com";

	while(true){
		DKIM * pdkim = vdkim.createDKIM_sign(jobid, status);
		dkim_sigkey_t key = vdkim.getKeyString();
		//printf("key:\n%s\n", (char *)key);
		
			
			status = vdkim.setDKIMHeader(pdkim, HEADER02);
			checkFlag(status, HEADER02);
			status = vdkim.setDKIMHeader(pdkim, HEADER03);
			checkFlag(status, HEADER03);
			status = vdkim.setDKIMHeader(pdkim, HEADER04);
			checkFlag(status, HEADER04);
			status = vdkim.setDKIMHeader(pdkim, HEADER05);
			checkFlag(status, HEADER05);
			status = vdkim.setDKIMHeader(pdkim, HEADER06);
			checkFlag(status, HEADER06);
			status = vdkim.setDKIMHeader(pdkim, HEADER07);
			checkFlag(status, HEADER07);
			status = vdkim.setDKIMHeader(pdkim, HEADER08);
			checkFlag(status, HEADER08);
			status = vdkim.setDKIMHeader(pdkim, HEADER09);
			checkFlag(status, HEADER09);
	
			status = vdkim.setDKIMEOH(pdkim);
	
			char body[]="test mail message boby";
	
			status = vdkim.setDKIMBody(pdkim, body);
			checkFlag(status, body);
			status = vdkim.setDKIMBody(pdkim, CRLF);
			checkFlag(status, CRLF);
			
			status = vdkim.setDKIMEOM(pdkim);
			checkFlag(status, CRLF);	
		
		
		char hdrbuf[SZ_DEFAULTBUFFER+1]= {0,};
		status = vdkim.getSignedHeader(pdkim, hdrbuf, SZ_DEFAULTBUFFER);

		status = dkim_free(pdkim);
	
		printf("dkimStat[%d]: Header[%s]\n=====>[%d]", status, hdrbuf, icount++);
		//return 0;
		icount++;
		usleep(20000);
		if(icount >10000)
			break;
	}
	time(&endt);
	printf("[Time : %d][Count: %d] [Avr:%f]\n", endt-startt,icount, (float)icount/(endt-startt));
	sleep(30);
	return 0;
}

