#ifndef _STH_STRING_H_
#define _STH_STRING_H_

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define isksc(c)   ( (unsigned char) (c) > (unsigned char) '\240'   && \
       (unsigned char)  (c) < (unsigned char) '\377' )
#define is7ksc(c)   ( (unsigned char) (c) > (unsigned char) '\040'   && \
       (unsigned char)  (c) < (unsigned char) '\177' )

#define THERE_IS_ALIEN 1

#define TRIMCRLF(str) if (THERE_IS_ALIEN && str != NULL && str[0] != '\0') { \
		char *__ptr=str+strlen(str)-1;	\
		while( __ptr!= NULL && *__ptr != '\0' && (*__ptr=='\r' || *__ptr=='\n')){	\
		*__ptr='\0'; __ptr--; }	\
}

#define RTRIM(str) if (THERE_IS_ALIEN && str != NULL && str[0] != '\0' ) { \
		char *__ptr=str+strlen(str)-1;\
		while( __ptr!= NULL && *__ptr != '\0' && isspace(*__ptr)){\
		*__ptr='\0'; __ptr--; }\
}

#define TRIM(str) if (THERE_IS_ALIEN && str != NULL && str[0] != '\0' ) { \
		char *__ptr=str+strlen(str)-1;\
		while( __ptr!= NULL && *__ptr != '\0' && isspace(*__ptr)){\
		*__ptr='\0'; __ptr--; }\
}

#define CKNULL(__s)	(__s?__s:"")
#define	ISNULL(__s) (__s?1:0)

#define	Free(__s) if(__s) free(__s); __s=NULL;
#define Close(__s) if(__s) {shutdown(__s, 2); close(__s);}
#define Fclose(__s) if(__s) fclose(__s); __s=NULL;

#define	TIME_TYPE_GMT		1
#define	TIME_TYPE_SESSION	2
#define	TIME_TYPE_NORMAL	3

  void *Malloc (const size_t size);
  void *Calloc (const size_t size);
  void *Realloc (void *pOld, const size_t NewSize);
  char *Strdup (const char *pOrg);
  void Strncpy (char *to, const char *from, const size_t size);
  void Strncat (char *to, const char *from, const size_t size);
  int Strncasecmp (const char *s1, const char *s2, const long n);
  int Strncmp (const char *s1, const char *s2, const long n);
  int Strcasecmp (const char *s1, const char *s2);
  char *Concat (const char *s, ...);
  char *LTRIM (char *s);
  char *QPrint (char *p);
  char *Base64Decode (char *str);
  void *BinBase64Decode (char *str, size_t * size);
  char *NowTimeToStr (int nFlag);
  char *MimeWordDecode (char *szSource);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_STRING_H_
