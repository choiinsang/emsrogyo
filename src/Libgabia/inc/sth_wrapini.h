/* *************************************************
�ۼ��� : ������
�ۼ��� : ?
��� ���� : INI �������� ������ �о� ���� �� �ִ�
************************************************* */
#ifndef _STH_WRAPINI_H_
#define _STH_WRAPINI_H_

#include <iniparser.h>

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

// INI ����
  typedef dictionary *INI_FILE;

/* *************************************************
INILoad:	INI �� �����Ѵ�.
INIGetBool:	Bool ������ �����͸� �д´�.
INIGetInt:	INT 
INIGetDouble:	Double
INIGetStr:	String
INIUnLoad:	INI �� �ݴ´�.
************************************************* */
#define	INILoad(__ini)						iniparser_load(__ini)
#define	INIGetBool(__ini, __key,__def)		iniparser_getboolean(__ini, __key,__def)
#define	INIGetInt(__ini, __key,__def)		iniparser_getint(__ini, __key,__def)
#define	INIGetDouble(__ini, __key,__def)	iniparser_getdouble(__ini, __key,__def)
#define	INIGetStr(__ini, __key,__def)		iniparser_getstring(__ini, __key,__def)
#define	INIUnLoad(__ini)					iniparser_freedict(__ini)

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_WRAPINI_H_
