#ifndef  _STH_MISC_H
#define  _STH_MISC_H

#include <sys/types.h>

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

  int CopyFile (const char *szSource, const char *szDest);
  int MoveFile (const char *szSource, const char *szDest, int isAppend);
  int MoveDirFile (const char *szSPath, const char *szDPath, int isRemove);
  int RemoveDirFile (const char *szPath);
  int CreateDir (const char *path);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_MISC_H
