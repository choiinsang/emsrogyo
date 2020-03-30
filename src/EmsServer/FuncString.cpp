/************************************************************************
** System: MailMan
** Program: MailMan 
** Comment:
*************************************************************************
** history
   2003.01.28 유성준 1차 작성 시작
   2005.11.03 박명주
				explode 함수 추가
************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h> 
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "FuncString.h"

pthread_mutex_t  mutex_util = PTHREAD_MUTEX_INITIALIZER;
static const char *arrayMonth[12] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
	};   
static const char *arrayWeek[7] = {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
	};

int string_replace(char* strSrc, int iSrc, char* strOld, char* strNew){
    char* strOldStart;
    int iOld, iNew;
    int iMove;

    iMove = 0;
    iOld = strlen(strOld);
    iNew = strlen(strNew);
    for(;;){
        if((strOldStart = (char *)strstr(strSrc + iMove, strOld)) == NULL){
            break;
        }
        if(((int)strlen(strSrc) + iNew - iOld + 1) > iSrc){
            return -1;
        }
        memmove(strOldStart + iNew, strOldStart + iOld, strlen(strOldStart + iOld) + 1);
        memcpy(strOldStart, strNew, iNew);
        iMove = strOldStart - strSrc + iNew;
    }
    return 0;
}

int string_replace_copy(char* strDes, int iMaxDes, char* strSrc, char* strOld, char* strNew){
    char* strOldStart;
    int iDes, iOld, iNew;

    strDes[0] = '\0';
    iDes = 0;
    iOld = strlen(strOld);
    iNew = strlen(strNew);
    for(;;){
        if((strOldStart = (char *)strstr(strSrc, strOld)) == NULL){
            break;
        }
        if(((int)strlen(strDes) + (int)strlen(strSrc) + iNew - iOld + 1) > iMaxDes){
            return -1;
        }
        memcpy(strDes + iDes, strSrc, strOldStart - strSrc);
        memcpy(strDes + iDes + (strOldStart - strSrc), strNew, iNew);
        strDes[iDes + strOldStart - strSrc + iNew] = '\0';
        iDes = iDes + strOldStart - strSrc + iNew;
        strSrc = strOldStart + iOld;
    }
    memcpy(strDes + iDes, strSrc, strlen(strSrc) + 1);
    return 0;
}

void printErr(const char* strFormat, ...){
    //time_t timeMark;
    //char strTime[256];
    //va_list ap;
    //char strBuffer[1024];
    //int iFile;

#ifdef _DEBUG

pthread_mutex_lock(&mutex_util); // 잠금을 생성한다.
    timeMark = time(NULL);
    strncpy(strTime, ctime(&timeMark), strlen(ctime(&timeMark)) - 1);
    strTime[strlen(ctime(&timeMark)) - 1] = '\0';
    printf("%s ", strTime);
    va_start(ap, strFormat);
//    vprintf(strFormat, ap);
    vsprintf(strBuffer, strFormat, ap);
    printf(strBuffer);
    fprintf(stderr, "%s ", strTime);
    fprintf(stderr, strBuffer);
    fflush(stdout);
    fflush(stderr);
    va_end(ap);

/*
    iFile = open("./result.txt", O_APPEND|O_WRONLY);
    if(iFile < 0) return; 
    write(iFile, strBuffer, strlen(strBuffer));
    close(iFile);
*/
pthread_mutex_unlock(&mutex_util); // 잠금을 해제한다.

#endif
//    fflush(stdout);
}

void getMonthName(char *strMonth, int iMonth){
    strcpy(strMonth, arrayMonth[iMonth]);
}

void getWeekName(char *strWeek, int iWeek){
    strcpy(strWeek, arrayWeek[iWeek]);
}

int get_self_ip(char* strIP){
    FILE *hFile; 
    char strBuffer[1024];

	if((hFile = fopen("/etc/sysconfig/network-scripts/ifcfg-eth0", "r")) == NULL){
		return -1;
	}
	while(fgets(strBuffer, 1024, hFile)!= NULL){
		if(!strncasecmp(strBuffer, "IPADDR=", strlen("IPADDR="))){
		    strcpy(strIP, (char *)(strchr(strBuffer, '=') + 1));
		    strIP[strlen(strBuffer) - 7] = '\0';
           	fclose(hFile);
			return 0;
		}
	}
	fclose(hFile);
	return -1;
}

char *to_sql_string(char* strSrc, int iMaxSrc){
    int iSrc;

    for(iSrc = 0;;){
        if(strSrc[iSrc] == '\0'){
            break;
        }
        if((strSrc[iSrc] == '\\') || (strSrc[iSrc] == '\'')){
            if(((int)strlen(strSrc) + 2) > iMaxSrc){
                break;
            }
            memmove(&(strSrc[iSrc + 1]), &(strSrc[iSrc]), strlen(&(strSrc[iSrc])) + 1);
            strSrc[iSrc++] = '\\';
        }
        iSrc++;
    }
    return strSrc;
}

void to_sql_string_copy(char* strDes, const char* strSrc){
    int iSrc;
    int iDes;
    
    iDes = 0;
    for(iSrc = 0; iSrc < (int)strlen(strSrc); iSrc++){
        if((strSrc[iSrc] == '\\') || (strSrc[iSrc] == '\'')){
            strDes[iDes] = '\\';
            iDes++;
        }
        strDes[iDes] = strSrc[iSrc];
        iDes++;
    }
    strDes[iDes] = '\0';
}

void to_lower_string(char* strSrc){
   int i = 0;

   while(strSrc[i]){
      strSrc[i] = tolower(strSrc[i]);
      i++;
   }
}

char **explode( char* str, char *need, int *count )
{
	/******************************************************************************
	Function name	: explode
	Parameter		: str (Target string)
					  need (Delemeter)
					  count (Array count - memory address return)
	Date			: 2005.11.03
	Return value	: ret (array)
	Version			: 1.0
	Made by 박명주
	******************************************************************************/
    int str_len;
    char **ret;
    char *cpy;
    char *tmp;
    int tmp_len = 0;
    int i;

    if ( str == (char *) 0 )
    {
        (*count) = 0;
        return (char **) 0;
    }

    str_len = strlen( str );
    if ( str_len == 0 )
    {
        (*count) = 0;
        return (char **) 0;
    }

    (*count) = 1;

    cpy = (char *) malloc( sizeof( char ) * str_len );
    cpy = strncpy( cpy, str, str_len );
    tmp = cpy;


    while ( 1 )
    {
        tmp=strstr( tmp, need );
        if ( tmp == (char *) 0)
            break;
        tmp[0] = '\0';
        tmp++;
        (*count)++;
    }

    ret = (char **) malloc( sizeof( char * ) * (*count) );

    for ( i=0; i < (*count); i++ )
    {
        ret[i] = &cpy[ tmp_len ];
        if ( ret[i] == (char *) 0 )
            tmp_len += 1;
        else
            tmp_len += strlen( ret[i] ) + 1;
    }

    return ret;
}


void ChangeNewline(char *_str)
{
	for(int i=0; i<(int)strlen(_str); i++)
	{
		//if(_str[i] == '\r' || _str[i] == '\n')
		if(_str[i] == '\'')
		{
			//m_Tmpstr[i] = ' ';
			_str[i] = '^';			
		}
	}
}

char b64string[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

long base64_encode (char *to, char *from, unsigned int len)
{
	char *fromp = from;
	char *top = to;
	unsigned char cbyte;
	unsigned char obyte;
	char end[3];

	for (; len >= 3; len -= 3) {
		cbyte = *fromp++;
		*top++ = b64string[(int)(cbyte >> 2)];
		obyte = (cbyte << 4) & 0x30;		/* 0011 0000 */

		cbyte = *fromp++;
		obyte |= (cbyte >> 4);			/* 0000 1111 */
		*top++ = b64string[(int)obyte];
		obyte = (cbyte << 2) & 0x3C;		/* 0011 1100 */

		cbyte = *fromp++;
		obyte |= (cbyte >> 6);			/* 0000 0011 */
		*top++ = b64string[(int)obyte];
		*top++ = b64string[(int)(cbyte & 0x3F)];/* 0011 1111 */
	}

	if (len) {
		end[0] = *fromp++;
		if (--len) end[1] = *fromp++; else end[1] = 0;
		end[2] = 0;

		cbyte = end[0];
		*top++ = b64string[(int)(cbyte >> 2)];
		obyte = (cbyte << 4) & 0x30;		/* 0011 0000 */

		cbyte = end[1];
		obyte |= (cbyte >> 4);
		*top++ = b64string[(int)obyte];
		obyte = (cbyte << 2) & 0x3C;		/* 0011 1100 */

		if (len) *top++ = b64string[(int)obyte];
		else *top++ = '=';
		*top++ = '=';
	}
	*top = 0;
	return top - to;
}


void RTrim(char *_pstr)
{
	int m_len = (int)strlen(_pstr);

	if(m_len > 0)
	{
		char *m_ptr = _pstr + m_len - 1;

		for(int i=0; i<m_len; i++)
		{
			if(*m_ptr == ' ')
				*m_ptr = '\0';
			else
				break;

			m_ptr--;
		}
	}
}


unsigned short _getshort(unsigned char *msgp)   
{   
	/*register */unsigned char *p = msgp;   
	/*register */unsigned short u;   

	u = *p++ << 8;   
	return ((u_short)(u | *p));   
}

char *strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') 
	{
		len = strlen(find);
		do 
		{
			do 
			{
				if ((sc = *s++) == '\0' || slen-- < 1)
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}


void to_lowercase(char *str)
{
	int i;
	for(i=strlen(str)-1; i>=0; i--){
		str[i]=tolower(str[i]);
	}
}


char *itostr(int iNumber, char *pBuf, int iBufSize)
{
	if((pBuf == NULL)||(iBufSize <= 0))
		return (char *)NULL;
		
	memset(pBuf, 0, iBufSize);
	snprintf(pBuf, iBufSize, "%d", iNumber);
	return pBuf;
}


