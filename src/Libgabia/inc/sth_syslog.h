#include <stdio.h>
#include <sys/syslog.h>
#include <stdarg.h>
#ifndef  _STH_SYSLOG_H_
#define  _STH_SYSLOG_H_

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

#define	PROCESS_LOCK_PATH	"/var/lock/subsys/%s"
#define	PROCESS_CMDLINE_PATH	"/proc/%d/cmdline"

  extern int STH_TR_SYSLOG;
  extern int STH_TR_VERBOSE;
  extern int STH_TR_LEVEL;

#define TR_ERROR	0	// Error Message Only
#define TR_WARNING	1	// Warning, Error Messages
#define TR_INFO		2	// Programmer Message Included
#define TR_DEBUG	3	// All Messages Showen

  void Trace (int level, char *szFormat, ...);
  void SetDebugOption (int level, int tr_syslog, int tr_verbose);
  void InitInetdDaemon (const char *pname, int facility);
  void InitDaemon (const char *pname, int facility);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_SYSLOG_H_
