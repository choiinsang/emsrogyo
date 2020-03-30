/************************************************************************
** System: MailMan
** Program: MailMan 
** Comment:
*************************************************************************
** history
   2003.01.28 유성준 1차 작성 시작
************************************************************************/
#pragma once
#include <stdlib.h>

void printErr(const char* strFormat, ...);
void getMonthName(char *strMonth, int iMonth);
void getWeekName(char *strWeek, int iWeek);
int get_self_ip(char* strIP);
char **explode( char* str, char *need, int *count );

void ChangeNewline(char *_str);

long base64_encode (char *to, char *from, unsigned int len);

void RTrim(char *_pstr);

unsigned short _getshort(unsigned char *msgp);

char *strnstr(const char *s, const char *find, size_t slen);

void to_lowercase(char *str);

char *itostr(int iNumber, char *pBuf, int iBufSize);


