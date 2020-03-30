#include "INIFile.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

CIniFile::CIniFile() : m_FP(NULL)
{
}

CIniFile::~CIniFile()
{
    EndINIFile();
}

/*===========================================================================================================
 * FUNCTION: _OpenFile
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   conf ������ ����.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszFile : [IN] conf ���ϸ�
 *   i_pszMode : [IN] ������ �� MODE ex) read only, write...
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   pFile : ������ conf ������ HANDLE��
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
FILE* CIniFile::_OpenFile(const char *i_pszFile, const char *i_pszMode)
{
    FILE *pFile;

    if (NULL == (pFile = fopen(i_pszFile, i_pszMode)))
        pFile = fopen(i_pszFile, "r+");
    return pFile;
}

/*===========================================================================================================
 * FUNCTION: InitINIFile
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   conf ������ ����.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszFileName : [IN] conf ���ϸ�
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   true  : conf ������ ���������� ������ ��
 *   false : conf ���� ���⸦ �������� ���
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
bool CIniFile::InitINIFile(const char *i_pszFileName)
{
    if ((m_FP = _OpenFile (i_pszFileName, "r+")) == NULL)
    {
        fprintf(stderr, "Error opening file %s - Aborting.\n", i_pszFileName);
        return false;
    }
    m_pszFileName = new char[strlen(i_pszFileName) + 1];
    strcpy(m_pszFileName, i_pszFileName);

    return true;
}

/*===========================================================================================================
 * FUNCTION: EndINIFile
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   conf ������ �ݴ´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
void CIniFile::EndINIFile()
{
    if (m_FP != NULL)
    {
        fclose(m_FP);
        m_FP = NULL;
    }
}

/*===========================================================================================================
 * FUNCTION: IsCommentLine
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   �ּ� ���������� �����Ѵ�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pLine : [IN] ������ Line
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   true  : �ּ������� ���
 *   false : �ּ������� �ƴ� ���
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::IsCommentLine(char *i_pLine)
{
    return (';' == i_pLine[0]);
}

/*===========================================================================================================
 * FUNCTION: GetProfileInt
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   INI���� �� Section�� Key���� �ش�Ǵ� integer���� �����´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszSection      : [IN] Section ��
 *   i_pKey            : [IN] Key ��
 *   i_iszDefaultValue : [IN] �ش� Ű���� ���� ��� Default ��
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   E_FILEREAD  : ���� �б⸦ �����Ͽ��� ���
 *   E_NOSECTION : Section ��ü�� conf�� ���� ���
 *   int         : �� Key�� ���� Integer Value
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::GetProfileInt(char *i_pszSection, char *i_pKey, int i_iszDefaultValue)
{
    char *pMatchValue = NULL;
    char  szLine[LINELEN];
    char  szTemp[LINELEN];
    int   iMatchSection(0);
    int   iMatchKey(0);

    rewind(m_FP);

    sprintf(szTemp, "[%s]", i_pszSection);
    while (fgets(szLine, LINELEN, m_FP))
    {
        if (!IsCommentLine(szLine))
        {
            if (!strncmp(szLine, szTemp, strlen(szTemp)))
            {
                iMatchSection = TRUE;
                break;
            }
        }
    }

    if (ferror(m_FP))
    {
        fprintf(stderr, "Error reading file - Aborting.\n");
        EndINIFile();
        return E_FILEREAD;
    }
    if (iMatchSection)
    {
        sprintf(szTemp, "%s=", i_pKey);

        while (fgets(szLine, LINELEN, m_FP))
        {
            if (!IsCommentLine(szLine))
            {
                if (strchr(szLine, '['))
                {
                    iMatchKey = FALSE;
    
                    return i_iszDefaultValue;
                    break;
                }
            }

            szLine[strlen(szLine)-1] = 0 ;
            if (!strncmp(szLine, szTemp, strlen(szTemp)))
            {
                iMatchKey = TRUE;
                pMatchValue = strchr(szLine, '=');
                pMatchValue++;

                return atoi(pMatchValue);
                break;
            }
        }
    }

    if (!iMatchSection || !iMatchKey)
    {
        return i_iszDefaultValue;
    }
    return 0;
}

/*===========================================================================================================
 * FUNCTION: GetProfileString
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   INI���� �� Section�� Key���� �ش�Ǵ� integer���� �����´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszSection      : [IN]  Section ��
 *   i_pKey            : [IN]  Key ��
 *   i_iszDefaultValue : [IN]  �ش� Ű���� ���� ��� Default ��
 *   i_pReturnValue    : [OUT] Return �� String ��
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   E_FILEREAD  : ���� �б⸦ �����Ͽ��� ���
 *   E_NOSECTION : Section ��ü�� conf�� ���� ���
 *   true        : ���������� Key�� ���� Value�� ��� ���� ���
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::GetProfileString(char *i_pszSection, char *i_pKey, char *i_pDefaultValue,
  char *i_pReturnValue)
{
    char  szTemp[LINELEN];
    char  szLine[LINELEN];
    char *pMatchValue = NULL;
    int   iMatchSection(0);
    int   iMatchKey(0);
    int   iLength(0);

    rewind(m_FP);
    sprintf(szTemp, "[%s]", i_pszSection);

    while (fgets(szLine, LINELEN, m_FP))
    {
        if (IsCommentLine(szLine))
            continue;
        if (!strncmp(szLine, szTemp, strlen(szTemp)))
        {
            iMatchSection = TRUE;
            break;
        }
    }

    if (ferror(m_FP))
    {
        fprintf(stderr, "Error reading file - Aborting.\n");
        EndINIFile();
        return E_FILEREAD;
    }

    if (iMatchSection)
    {
        sprintf(szTemp, "%s=", i_pKey);
        while (fgets(szLine, LINELEN, m_FP))
        {
            if (IsCommentLine(szLine))
                continue;
            if (strchr(szLine, '['))
            {
                iMatchKey = FALSE;
                strcpy(i_pReturnValue, i_pDefaultValue);

                return E_NOKEY ;
            }

            szLine[strlen(szLine) - 1] = 0 ;
            if (!strncmp(szLine, szTemp, strlen(szTemp)))
            {
                iMatchKey = TRUE;
                pMatchValue = strchr(szLine, '=');
                pMatchValue++;
                strcpy(i_pReturnValue, pMatchValue);
                iLength = strlen(i_pReturnValue);
                if (i_pReturnValue[iLength - 1] == '\r')
                    i_pReturnValue[iLength - 1] = 0x00;

                return TRUE;
            }   // strncmp
        }   // while
    }   // match_section
    else
    {
        strcpy(i_pReturnValue, i_pDefaultValue);
        return E_NOSECTION ;
    }

    if (!iMatchSection || !iMatchKey)
    {
        strcpy(i_pReturnValue, i_pDefaultValue);
        return TRUE;
    }
    return 0;
}

/*===========================================================================================================
 * FUNCTION: GetProfileKeyList
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *   INI���� �� Section�� Key���� �ش�Ǵ� integer���� �����´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszSection      : [IN]  Section ��
 *   i_pKey            : [IN]  Key ��
 *   i_iszDefaultValue : [IN]  �ش� Ű���� ���� ��� Default ��
 *   i_pReturnValue    : [OUT] Return �� String ��
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   E_FILEREAD  : ���� �б⸦ �����Ͽ��� ���
 *   E_NOSECTION : Section ��ü�� conf�� ���� ���
 *   true        : ���������� Key�� ���� Value�� ��� ���� ���
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::GetProfileKeyList(char *i_pszKey, char *i_pszDefaultValue, char *i_pszReturnValue,
  int* i_piReturnValueSize)
{
    char  szTemp[LINELEN];
    char  szLine[LINELEN];
    char *pMatchValue = NULL;
    char  szKeyList[4096] = "";
    int   iKeyCount(0);
    int   iMatchSection(0);

    *i_piReturnValueSize = 0;

    rewind(m_FP);
    sprintf(szTemp, "[%s]", i_pszKey);

    while (fgets(szLine, LINELEN, m_FP))
    {
        if (!IsCommentLine(szLine))
        {
            if (!strncmp(szLine, szTemp, strlen(szTemp)))
            {
                iMatchSection = TRUE;
                break;
            }
        }
    }

    if (ferror(m_FP))
    {
        fprintf(stderr, "Error reading file - Aborting.\n");
        EndINIFile();
        return E_FILEREAD;
    }
    
    if (iMatchSection)
    {
        while (fgets(szLine, LINELEN, m_FP))
        {
            if (!IsCommentLine(szLine))
            {
                if (strchr(szLine, '['))
                {
                    memcpy(i_pszReturnValue, szKeyList, *i_piReturnValueSize);
                    return iKeyCount ;
                }   
    
                pMatchValue = strchr(szLine, '=');
                if (pMatchValue)
                {
                    pMatchValue[0] = 0 ;
                    memcpy(&szKeyList[*i_piReturnValueSize], szLine, strlen(szLine) + 1);
                    *i_piReturnValueSize = *i_piReturnValueSize + strlen(szLine) + 1;
                    iKeyCount++ ;
                }
            }
        }   // while
    }   // match_section
    else
    {
        strcpy(i_pszReturnValue, i_pszDefaultValue);
        return iKeyCount;
    }

    if (iKeyCount == 0)
        strcpy(i_pszReturnValue, i_pszDefaultValue);
    else
        memcpy(i_pszReturnValue, szKeyList, *i_piReturnValueSize);

    return iKeyCount ;
}

/*===========================================================================================================
 * FUNCTION: GetProfileKeyValueList
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszKey            : [IN]  Key ��
 *   i_pszDefaultValue   : [IN]  Default ��
 *   i_pszReturnValue    : [OUT]
 *   i_piReturnValueSize : [OUT]
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::GetProfileKeyValueList(char *i_pszKey, char *i_pszDefaultValue, char *i_pszReturnValue,
  int* i_piReturnValueSize)
{
    int   iRetCode;
    char *pKey;
    int   iKeyCount(0);
    int   iRetSize(0);
    char  szKeyList[4096];
    char  szKeyValue[LINELEN];

    memset(szKeyList, 0x00, 4096);
    memset(szKeyValue, 0x00, LINELEN);

    *i_piReturnValueSize = 0;

    iKeyCount = GetProfileKeyList(i_pszKey, "", szKeyList, &iRetSize) ;
    if (iKeyCount != 0)
    {
        pKey = szKeyList;
        for (int i = 0; i < iKeyCount; i++)
        {
            iRetCode = GetProfileString(i_pszKey, pKey, "", szKeyValue);
            if (iRetCode == I_SUCCESS)
            {
                memcpy(&i_pszReturnValue[*i_piReturnValueSize], pKey, strlen(pKey));
                *i_piReturnValueSize += strlen(pKey);
                i_pszReturnValue[*i_piReturnValueSize] = 0;    // null seperator
                *i_piReturnValueSize += 1;
                memcpy(&i_pszReturnValue[*i_piReturnValueSize], szKeyValue, strlen(szKeyValue));
                *i_piReturnValueSize += strlen(szKeyValue);
                i_pszReturnValue[*i_piReturnValueSize] = 0;    // null seperator
                *i_piReturnValueSize += 1;
            }

            pKey = pKey + strlen(pKey) + 1;
        }
    }
    else
        strcpy(i_pszReturnValue, i_pszDefaultValue);

    return iKeyCount;
}

/*===========================================================================================================
 * FUNCTION: SetProfileKeyValueList
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 *
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszKey     : [IN]  Key ��
 *   i_iKeyCount  : [IN]
 *   i_pszKeyList : [IN]
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int CIniFile::SetProfileKeyValueList(char *i_pszKey,int i_iKeyCount, char *i_pszKeyList)
{
    int   iRetCode;
    int   iRetKeyCount(0);
    char *pKey;
    char  szKeyName[LINELEN];
    char  szKeyValue[LINELEN];

    memset(szKeyName, 0x00, LINELEN);
    memset(szKeyValue, 0x00, LINELEN);

    if (i_iKeyCount != 0)
    {
        pKey = i_pszKeyList;
        for (int i = 0; i < i_iKeyCount; i++)
        {
            strcpy(szKeyName, pKey) ;
            pKey = pKey + strlen(szKeyName) + 1 ;
            strcpy(szKeyValue, pKey) ;
            pKey = pKey + strlen(szKeyValue) + 1 ;

            iRetCode = SetProfileString(i_pszKey, szKeyName, szKeyValue);
            if (iRetCode == I_SUCCESS)
                iRetKeyCount++ ;
        }
    }

    return iRetKeyCount;
}

/*===========================================================================================================
 * FUNCTION: SetProfileInt
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 * �ش� Section�� Key�� ���� Integer ���� �ִ´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszSection : [IN] Section ��
 *   i_pszKey     : [IN] Key ��
 *   i_iValue     : [IN] ���� Integer ��
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::SetProfileInt(char *i_pszSection, char *i_pszKey, int i_iValue)
{
    char szTemp[LINELEN];
    sprintf(szTemp, "%d", i_iValue);

    return SetProfileString(i_pszSection, i_pszKey, szTemp);
}

/*===========================================================================================================
 * FUNCTION: SetProfileString
 *-----------------------------------------------------------------------------------------------------------
 * DESCRIPTION
 * �ش� Section�� Key�� ���� string ���� �ִ´�.
 *-----------------------------------------------------------------------------------------------------------
 * PARAMETERS
 *   i_pszSection : [IN] Section ��
 *   i_pszKey     : [IN] Key ��
 *   i_pszValue   : [IN] ���� string ��
 *-----------------------------------------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * RETURN VALUE
 *   
 *-----------------------------------------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2005. 07. 06 : Created (bestchoice)
===========================================================================================================*/
int  CIniFile::SetProfileString(char *i_pszSection, char *i_pKey, char *i_pszValue)
{
    char  szLine[LINELEN];
    char  szTemp1[LINELEN];
    char  szTemp2[LINELEN];
    FILE *pFile;
    int   iInSection(FALSE);
    int   iMatchSection(FALSE);
    int   iDone(FALSE);
    char *szNewFile;

    szNewFile = new char[strlen(m_pszFileName) + 1];

    if (!szNewFile)
    {
        fprintf(stdout, "Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    strcpy(szNewFile, m_pszFileName);
    szNewFile[strlen(szNewFile) - 1] = '~';

    if (NULL == (pFile = _OpenFile(szNewFile, "r+")))
    {
        fprintf(stdout, "set_profile_string: Error opening file %s - Aborting\n", szNewFile);
        delete [] szNewFile;
        return E_FILEOPEN;
    }

    rewind(m_FP);

    sprintf(szTemp1, "[%s]", i_pszSection);
    sprintf(szTemp2, "%s=", i_pKey);

    while (fgets(szLine, LINELEN, m_FP))
    {
        // add new key and value immediately after section ID
        if (!iDone && !strncmp(szLine, szTemp1, strlen(szTemp1)))
        {
            iInSection = TRUE;
            iMatchSection = TRUE;

            fputs(szLine, pFile);
            sprintf(szTemp1, "%s=%s\n", i_pKey, i_pszValue);
            fputs(szTemp1, pFile);
            // reset buffer
            sprintf(szTemp1, "[%s]", i_pszSection);
            iDone = TRUE;
            continue;
        }
        // don't add the key to be replaced to new file
        if (iInSection && (!strncmp(szLine, szTemp2, strlen(szTemp2))))
            continue;

        if (iInSection && strchr(szLine, '['))
            iInSection = FALSE;

        fputs(szLine, pFile);
    }   // while
    if (ferror(m_FP))
    {
        fprintf(stdout, "Error reading file - Aborting.\n");
        delete [] szNewFile;
        fclose(pFile);
        return E_FILEREAD;
    }

    if (!iMatchSection)
    {
        fprintf(pFile, "[%s]\n%s=%s\n", i_pszSection, i_pKey, i_pszValue);
    }
    fclose(m_FP);
    fclose(pFile);
    remove(m_pszFileName); // delete old file
    rename(szNewFile, m_pszFileName); // rename new to old
    pFile = _OpenFile(m_pszFileName, "r+");

    // update FILEINFO array
    m_FP = pFile;
    delete [] szNewFile;

    return I_SUCCESS;
}
