#ifndef __CDMARCXMLPARSER_THREAD_HEADER__
#define __CDMARCXMLPARSER_THREAD_HEADER__

//--------------------------------------------------
// DMARC XML Parser Thread
//--------------------------------------------------
#include "CDMARCXmlParser.h"
#include "CDMARCMysql.h"


class CDMARCXmlParserThread
{
public:
	            ~CDMARCXmlParserThread();

	bool         isStop               ()           { return m_bIsStop; };
	void         setRunState          (int iState) { m_iRunState = iState; };
	int          getRunState          ()           { return m_iRunState ; };
	void         startThread          ();

	static void                  *xmlParserThreadFunc (void *Param);	//Message Process Function
	static CDMARCXmlParserThread *getInstance         ();


	shared_ptr<CDMARCXmlParser> parseFile (const char *filePath, int extNo);
	
	bool        moveFile             (const char *fromPath, const char *toPath, const char *fileName);

	bool        ConnectDB            ();
	
	bool        Start_Transaction    ();
	bool        Commit_Transaction   ();
	bool        Rollback_Transaction ();

	bool        InsertIntoDB         (shared_ptr<CDMARCXmlParser> xmlInfo);


	char       *getOrgDirPath        () { return m_chOrgDirPath; }
	char       *getTmpFileDir        () { return m_chTmpDirPath; }
	char       *getTargetDirPath     () { return m_chTargetDirPath; }
	char       *getErrorDirPath      () { return m_chErrorDirPath; }

private:
	            CDMARCXmlParserThread();
	            
private:
	static CDMARCXmlParserThread *m_pDMARCXmlParserThread;

	pthread_t   m_tParserThread;
	bool        m_bIsStop;      // Thread State
	int         m_iRunState;
	char        m_chOrgDirPath   [SZ_NAME+1];
	char        m_chTmpDirPath   [SZ_NAME+1];
	char        m_chTargetDirPath[SZ_NAME+1];
	char       	m_chErrorDirPath [SZ_NAME+1];
	
	CDMARCMysql m_DmarcMysql;
	CDMARCMysql m_DmarcMysqlSub;
	
};

#define theXmlParserThread() CDMARCXmlParserThread::getInstance()

#endif //__CDMARCXMLPARSER_THREAD_HEADER__
