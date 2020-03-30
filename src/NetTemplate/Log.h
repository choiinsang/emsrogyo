
#pragma once
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdarg>
//#include <string>
#include <string.h>


#include "LockObject.h"
#include "DefineNetGlobal.h"

class CLog : public LockObject
{
// Construction
public:
    ~CLog();
private:
    CLog();

// Overrides
public:

// Implementation
private:
    static CLog *m_pLog;
    FILE        *m_FPLog;
    char        m_szPath[MAX_PATH + 1];
    int         m_nLastDay;

	char		mCheckerCh;

public:
    static CLog &GetInstance(char _ch=' ');
    bool InitLog(char *l_pszPath);
    void Write(const char *l_pszFMT, ... );
    void WriteLOG(const char *l_pszData);

	void Set_CheckerCh(char _ch);
private:
};

//#define VERSION_TEST

//¡÷ºÆ
#define SM //##/
#define SM_N //NetTemplate //
#define SM_Q //EmsQueue //
#define SM_M //EmsMx //
#define SM_C //EmsClient
#define SM_T //EmsThread
#define SM_S //EmsServer
#define SM_D //DB
#define SM_E //ERROR
#define SM_A //Always
#define SM_L //Campain,Mail List




