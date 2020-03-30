#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
//#include <pcre/pcre.h>
#include "sth_syslog.h"

void *
Malloc(const size_t size)
{
	void *ret;
	ret = malloc(size);
	if (ret == NULL)
		Trace(TR_ERROR,"Error Malloc");
	return ret;
}

void *
Calloc( const size_t size)
{
	void *ret;
	ret = calloc(1,size);
	if (ret == NULL)
		Trace(TR_ERROR,"Error Calloc");
	return ret;
}

void *
Realloc( void * pOld, const size_t NewSize)
{
	void *ret;
	ret = realloc(pOld,NewSize);
	if (ret == NULL)
		Trace(TR_ERROR,"Error Realloc");
	return ret;
}

char *
Strdup(const char * pOrg)
{
	char *ret;
	size_t s;
	if (pOrg == NULL)
		return NULL;
	s = strlen(pOrg) + 1;

	if( (ret = Malloc(s)) == NULL)
		return NULL;

	memcpy(ret,pOrg,s);
	return(ret);
}

void
Strncpy(char * to, const char * from, const size_t size)
{
	strncpy(to,from,size);
	to[size]= '\0';
	return;
}

void
Strncat(char * to, const char * from, const size_t size)
{
	size_t l;
	l = strlen(to);
	Strncpy(&to[l],from,size);
	return;
}

int
Strncasecmp(const char * s1, const char * s2, const long n)
{
	long s;
	for (s = 0; s < n; s++) {
		if (tolower((unsigned char) s1[s]) != tolower((unsigned char) s2[s])) {
			if (tolower((unsigned char) s1[s]) < tolower((unsigned char) s2[s]))
				return -1;
			else
				return 1;
		}
		if (s1[s] == '\0')
			return 0;
	}
	return 0;
}

int
Strncmp(const char * s1, const char * s2, const long n)
{
	long s;
	for (s = 0; s < n; s++) {
		if ( s1[s] != s2[s])
		{
			if(s1[s] < s2[s])
				return -1;
			else
				return 1;
		}
		if (s1[s] == '\0')
			return 0;
	}
	return 0;
}

int
Strcasecmp(const char * s1, const char * s2)
{
	size_t s;
	char c1, c2;
	for (s = 0; (s1[s] != '\0') && (s2[s] != '\0') && (tolower((unsigned char) s1[s]) == tolower((unsigned char) s2[s])); s++);

	c1 = tolower((unsigned char) s1[s]);
	c2 = tolower((unsigned char) s2[s]);
	if (c1 == c2)
		return 0;
	else if (c1 < c2)
		return -1;
	else
		return 1;
}

char *
Concat(const char * s, ...) {
	const char *curr;
	size_t len = 0;
	va_list va;
	char * ret = NULL;

	va_start(va,s);
	curr = s;
	while (curr != NULL) {
		len += strlen(curr);
		curr = va_arg(va, const char *);
	}
	va_end(va);

	if (len) {
		ret = Malloc(len + 1);
		if( ret == NULL ) return NULL;
		*ret = '\0';
	}

	va_start(va,s);
	curr = s;
	while (curr != NULL) {
		strcat(ret,curr);
		curr = va_arg(va, const char *);
	}
	va_end(va);

	return ret;
}

char *
LTRIM(const char * s)
{
	char *ret;
	ret = (char *)s;

	if (ret != NULL) {
		while (isspace(*ret))
			ret++;
	}
	return ret;
}


int _sf_in_mime_words;

char *
QPrint(char *p) {
	char *buf;
	char *s;
	register int n;

	if(!p) return NULL;

        s=buf=(char *)Malloc(strlen(p) + 1);
        if(!buf)
                return NULL;

	for(; *p; p++) {
		if(_sf_in_mime_words && (*p == '_')) {
			*s++ = ' ';
			continue;
		};

		if(*p == '=') {
			*s=0;
			n=*++p;

			if(!n) { p--; break; }

                        if(!*(p+1)) break;

                        if( n >= '0' && n <= '9') *s=n-'0';
			else if( (n >= 'A' && n <= 'F') || (n >= 'a' && n <= 'f') )
				*s=10+n-((n>='a')?'a':'A');
			else {
				if(n == '\n' || n == '\r') continue;
				*s++ = '='; *s++ = n; continue;
                        };

			*s*=16;
			n=*++p;

			if( n >= '0' && n <= '9') *s += n-'0';
			else if( (n >= 'A' && n <= 'F') || (n >= 'a' && n <= 'f') )
				*s += 10+n-((n>='a')?'a':'A');
			else {
				if(n == '\n' || n == '\r') { *s++ = n; continue; };
                                *s = ' '; continue;
                        };

                        s++;
                        continue;
		};

		*s++=*p;
	}
	*s=0;

	return buf;
};

static unsigned char
_sf_uc_bi[255] = {
	255, /* '\0' */
100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
	62, /* + */
	100, 100, 100,
	63, /* / */
	/* 0-9 */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	100, 100, 100,
	255, /* = */
	100, 100, 100,
	/* A-Z */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	100, 100, 100, 100, 100, 100,
	/* a-z */
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100
};

char *
Base64Decode(char *str)
{
	char *output, *ou;

	if(!str) return NULL;

	ou = output = (void *)Malloc(strlen(str) + 1);
	if(!output)
		return NULL;

{
	unsigned int pr[6];
	register int n;
	register int doit=1;

	while(doit) {
		n=0;
		while(n < 4) {
			register unsigned char ch;
			ch=_sf_uc_bi[*(unsigned char *)str];
			if(ch < 100) { pr[n]=ch; n++; str++; continue; };
			if(ch == 100) { str++; continue; };
			doit=0; break;
		};
		if(!doit && n < 4) { pr[n+2]=pr[n+1]=pr[n]=0; };

		*(char *)ou=(pr[0] << 2) | (pr[1] >> 4);
		*(char *)(ou+1)=(pr[1] << 4) | (pr[2] >> 2);
		*(char *)(ou+2)=(pr[2] << 6) | pr[3];

		ou+=(n * 3) >> 2 ;

	};

	*(char *)ou=0;
}

	return output;
};

void *
BinBase64Decode(char *str, size_t *size) {
	void *output, *ou;

	if(size) *size=0;
	if(!str) str = "";

	ou = output = (void *)Malloc(strlen(str) + 1);
	if(!output)
		return NULL;
{
	unsigned int pr[6];
	register int n;
	register int doit=1;

	while(doit) {
		n=0;
		while(n < 4) {
			register unsigned char ch;
			ch=_sf_uc_bi[*(unsigned char *)str];
			if(ch < 100) { pr[n]=ch; n++; str++; continue; };
			if(ch == 100) { str++; continue; };
			doit=0; break;
		};
		if(!doit && n < 4) { pr[n+2]=pr[n+1]=pr[n]=0; };

		*(char *)ou=(pr[0] << 2) | (pr[1] >> 4);
		*(char *)(ou+1)=(pr[1] << 4) | (pr[2] >> 2);
		*(char *)(ou+2)=(pr[2] << 6) | pr[3];

		ou+=(n * 3) >> 2 ;

	};

	*(char *)ou=0;
}

	if(size) *size = (ou - output);
	return output;
};

char *
NowTimeToStr(int nFlag)
{
	char *szTime;
	struct tm *tmLocalTime;
	time_t ttTemp;
	static char *wd[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static char *mn[]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul","Aug", "Sep", "Oct", "Nov", "Dec" };

	ttTemp = time((time_t *) 0);
	tmLocalTime = localtime(&ttTemp);

	if( nFlag == 2 )
	{
		szTime = (char *)Malloc(30);

		// 현재 시각을 String 으로 변환하여 집어 넣는다.
		sprintf(szTime,"%04d%02d%02dT%02d%02d%02d+0900",
			tmLocalTime->tm_year+ 1900,
			tmLocalTime->tm_mon+1,
			tmLocalTime->tm_mday,
			tmLocalTime->tm_hour,
			tmLocalTime->tm_min,
			tmLocalTime->tm_sec);
	}
	else if( nFlag == 3 )
	{
		szTime = (char *)Malloc(30);

		// 현재 시각을 String 으로 변환하여 집어 넣는다.
		sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",
			tmLocalTime->tm_year+ 1900,
			tmLocalTime->tm_mon+1,
			tmLocalTime->tm_mday,
			tmLocalTime->tm_hour,
			tmLocalTime->tm_min,
			tmLocalTime->tm_sec);
	}
	else
	{
		//Tue, 21 Jan 2003 10:56:48 +0900
		szTime = (char *)Malloc(40);

		// 현재 시각을 String 으로 변환하여 집어 넣는다.
		sprintf(szTime,"%s, %d %s %d %02d:%02d:%02d +0900",
			wd[tmLocalTime->tm_wday],
			tmLocalTime->tm_mday,
			mn[tmLocalTime->tm_mon],
			tmLocalTime->tm_year+ 1900,
			tmLocalTime->tm_hour,
			tmLocalTime->tm_min,
			tmLocalTime->tm_sec);
	}

	return szTime;
}
/*
char *
MimeWordDecode( char *szSource )
{
	pcre *re;
	const char *error;
	const char szEx[] = "=\\?([a-zA-Z0-9._-]+)\\?(.)\\?([^[:space:]\n\r\t?]+)\\?=[ \n\r\t]*";
	int erroffset;
	int rc;
	int ovector[30];
	char *pTemp,*pTemp2;
	int nLen;
	char *ret;
	char *pRet=ret;

	if( !szSource ) return NULL;

	ret = (char *)Malloc(strlen(szSource+1));
	if(!ret) Strdup(szSource);

	*ret=0;

	nLen=strlen(szSource);
	pTemp = szSource;

	re = pcre_compile(szEx,
        0,
        &error,
        &erroffset,
        NULL);

	if( !re )
	{
		free(ret);
		return Strdup(szSource);
	}

	while(1)
	{
		char **szReturn;
		int nZeroLen;
		char *szTempEnc;

		rc = pcre_exec(re,
			NULL,
			pTemp,
			strlen(pTemp),
			0,0,
			ovector,30);

		if( rc != 4 )
		{
			pcre_free(re);
			if( *ret )
			{
				strcat(ret,pTemp);
				return ret;
			}
			else return strdup(szSource);
		}

		pcre_get_substring_list( pTemp, ovector, rc, (const char ***)&szReturn );
		nZeroLen = strlen(szReturn[0]);

		if( !Strcasecmp(szReturn[2],"B") )
			szTempEnc = Base64Decode(szReturn[3]);
		else if( !Strcasecmp(szReturn[2],"Q") )
			szTempEnc = QPrint(szReturn[3]);
		else
		{
			pcre_free(re);
			if( *ret )
			{
				strcat(ret,pTemp);
				return ret;
			}
			else return strdup(szSource);
		}

		pTemp2 = strstr(pTemp,szReturn[0]);

		if( *ret )
			Strncat(ret,pTemp,pTemp2-pTemp);
		else
			Strncpy(ret,pTemp,pTemp2-pTemp);

		strcat(ret,szTempEnc);
		pTemp=pTemp2+strlen(szReturn[0]);
		nLen=pTemp-szSource;

		pcre_free_substring_list((const char **)szReturn);

		if(nLen<=0) break;
	}

	pcre_free(re);
	return ret;
}

*/
