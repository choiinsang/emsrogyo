#ifndef  _LIBGABIA_H
#define  _LIBGABIA_H

#include <stdio.h>
#include <sys/syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#include "sth_syslog.h"
#include "sth_list.h"
#include "sth_misc.h"
#include "sth_string.h"
#include "sth_mysql.h"
#include "sth_network.h"
#ifdef	_HAVE_INIPARSER_H_
#include "sth_wrapini.h"
#endif

#endif // _LIBGABIA_H
