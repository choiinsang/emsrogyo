#ifndef __CDMARCXMLPARSER_HEADER__
#define __CDMARCXMLPARSER_HEADER__

#include <sstream>
#include <string.h>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "DMARCDefineHeader.h"

using namespace std;
using namespace boost;

//DMARC XML DATA INFORMATION STRUCTURE
//xml:feedback(1dep)->report_metadata(2dep)
typedef struct _stReportMetaData{
	string m_strOrgName;
	string m_strEmail;
	string m_strExtraContactInfo;
	string m_strReport_id;
	string m_strBegin;
	string m_strEnd;
}stReportMetaData;

//xml:feedback(1dep)->policy_published(2dep)
typedef struct _stPolicyPublished {
	string m_strDomain;
	string m_strAdkim;
	string m_strAspf;
	string m_strP;
	string m_strSP;
	string m_strPct;
}stPolicyPublished;

//xml:feedback(1dep)->record(2dep)->row(3dep)->policy_evaluated(4dep)
typedef struct _stPolicyEvaluated {
	string m_strDisposition;
	string m_strDkim;
	string m_strSpf;
}stPolicyEvaluated;

//xml:feedback(1dep)->record(2dep)->row(3dep)->auth_result(4dep)->(dkim, spf)(5dep)
typedef struct _stAuthResultsDomainResult {
	_stAuthResultsDomainResult(){
		m_strDomain = string("");
		m_strResult = string("");
		m_bExist    = false;
	}
	bool   isExist  ()            { return m_bExist; }
	void   setExist (bool bExist) { m_bExist = bExist; }
	
	public:
		string m_strDomain;
		string m_strResult;

	private:
		bool   m_bExist;
}stAuthResultsDomainResult;

//xml:feedback(1dep)->record(2dep)->row(3dep)->auth_result(4dep)
typedef struct _stAuthResults {
	stAuthResultsDomainResult *getAuthResultSpf () { return &m_stAuthResultSpf;  }
	stAuthResultsDomainResult *getAuthResultDkim() { return &m_stAuthResultsDkim; }
	
	private:
		stAuthResultsDomainResult m_stAuthResultSpf;
		stAuthResultsDomainResult m_stAuthResultsDkim;
}stAuthResults;

//xml:feedback(1dep)->record(2dep)->row(3dep)
typedef struct _stRow {
	_stRow(){
		m_uiCount     = 0;
		m_strSourceIP = string("0.0.0.0");
	}
	
	stPolicyEvaluated *getpPolicyEvaluated() { return &m_stPolicyEvaluated; }
	
	public:
		uint   m_uiCount;
		string m_strSourceIP;
	
	private:
		stPolicyEvaluated m_stPolicyEvaluated;	
}stRow;

//xml:feedback(1dep)->record(2dep)->row(3dep)->identifires(4dep)
typedef struct _stIdentifires {
	string m_strHeaderFrom;
}stIdentifires;

//xml:feedback(1dep)->record(2dep)
typedef struct _stRecord {
	public:
		stRow         *getRow        () { return &m_stRow;  }
		stIdentifires *getIdentifires() { return &m_stIdentifires; }
		stAuthResults *getAuthResults() { return &m_stAuthResults; }
	private:
		stRow         m_stRow;
		stIdentifires m_stIdentifires;
		stAuthResults m_stAuthResults;
}stRecord;

//xml:feedback(1dep)
typedef struct _stFeedback {
	_stFeedback(){
		m_stRecords = shared_ptr<vector<shared_ptr<stRecord> > >(new vector<shared_ptr<stRecord> >);
	};
	
	stReportMetaData  *getReportMetaData () { return &m_stReportMetaData; }
	stPolicyPublished *getPolicyPublished() { return  &m_stPolicyPublished; }
	
	shared_ptr<vector<shared_ptr<stRecord> > > getRecords() { return m_stRecords; }
	

	private:
	stReportMetaData  m_stReportMetaData;
	stPolicyPublished m_stPolicyPublished;
	shared_ptr<vector<shared_ptr<stRecord> > > m_stRecords;
}stFeedbackRecord;


class CDMARCXmlParser
{
	public:
		CDMARCXmlParser();
		~CDMARCXmlParser();

		bool LoadXml  (std::string xmlFilePath);
		bool ParseXml (std::string xmlFilePath);
			
		int  fileExtract(std::string zipFilePath, std::string &retPath);
		bool fileParse  (std::string orgPath,     std::string &retPath);
		bool ParseZip   (std::string xmlFilePath);
			

		shared_ptr<stFeedbackRecord> getFeedbackRecord() { return m_spFeedbackRecord; }
		bool                         getXmlLoadState  () { return m_bXmlLoadState; }

	private:
		bool                         m_bXmlLoadState;
		std::string                  m_strXMLFileName;
		std::stringstream            m_strXMLData;
		shared_ptr<stFeedbackRecord> m_spFeedbackRecord;

};

#endif //__CDMARCXMLPARSER_HEADER__

