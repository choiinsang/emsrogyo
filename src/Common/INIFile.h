

#ifndef __INIFILE_H
#define __INIFILE_H

#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifndef TRUE
#define TRUE                              1
#endif

#ifndef FALSE
#define FALSE                             0
#endif

enum _INIRESULT
{
    I_SUCCESS      =  1,
    I_ERROR        = -1, // General error condition
    E_OUTOFMEMORY  = -2, // malloc() failed
    E_FILEOPEN     = -3, // Error opening a file
    E_FILEREAD     = -4, // Error reading a file
    E_NOSECTION    = -5, // delete_profile_key: Requested section could not be found
    E_NOKEY        = -6, // delete_profile_key: Requested key could not be found
    E_NOOPENFILE   = -7, // INIFILE was not properly initialized
    E_INVALIDINDEX = -8, // function parameter idx not identical to that returned from init_inifile()
    E_TOOMANYFILES = -9, // too many registered inifiles
};

#define LINELEN 512

class CIniFile
{
// Construction
public:
    CIniFile();
    virtual ~CIniFile();

// Attributes
public:
    FILE* m_FP;
    char* m_pszFileName;

// Operations
private:
	int    IsCommentLine (char *i_pLine);
	int    GetProfileKeyValueList(char *i_pszKey, char *i_pszDefaultValue, char *i_pszReturnValue,
	                              int* i_piReturnValueSize);
	int    GetProfileKeyList(char *i_pszKey, char *i_pszDefaultValue, char *i_pszReturnValue,
	                        int* i_piReturnValueSize);
	int    SetProfileKeyValueList(char *i_pszKey, int i_iKeyCount, char *i_pszKeyList);
	FILE*  _OpenFile(const char *i_pszFile, const char *i_pszMode);

public:
    bool InitINIFile(const char *i_pszFileName);
    void EndINIFile();
    int  GetProfileInt   (char *i_pszSection, char *i_pKey,   int i_iszDefaultValue);
    int  GetProfileString(char *i_pszSection, char *i_pKey,   char *i_pDefaultValue, char *i_pReturnValue);
    int  SetProfileInt   (char *i_pszSection, char *i_pszKey, int i_iValue);
    int  SetProfileString(char *i_pszSection, char *i_pKey,   char *i_pszValue);
};
#endif //__INIFILE_H
