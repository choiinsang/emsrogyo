bin_PROGRAMS = DMARCXmlParser


DMARCXmlParser_CPPFLAGS = -Wall -g -O -D__LINUX__ -D_DAEMON_VERSION_=0 -D_Bool=bool -I/usr/local/include  -I../boost_1_41_0  -I../NetTemplate  -I../Libgabia/inc 
DMARCXmlParser_LDFLAGS  = -L../Libgabia/lib -lgabia -lm -lpthread -L/usr/lib64/mysql -lmysqlclient -liconv -lzip
DMARCXmlParser_SOURCES  = DMARCXmlParserMain.cpp  \
		../Common/INIFile.h ../Common/INIFile.cpp  \
		../NetTemplate/LockObject.h ../NetTemplate/LockObject.cpp \
		../NetTemplate/DefineNetGlobal.h \
		../NetTemplate/Log.h  ../NetTemplate/Log.cpp \
		../Libgabia/src/sth_syslog.h ../Libgabia/src/sth_syslog.c \
		DMARCDefineHeader.h  DMARCDefineString.h\
		CDMARCFunctions.h CDMARCFunctions.cpp  \
		CDMARCXmlParserConfig.h CDMARCXmlParserConfig.cpp \
		CDMARCLog.h CDMARCLog.cpp \
		CDMARCMysql.h CDMARCMysql.cpp \
		CDMARCXmlParser.h CDMARCXmlParser.cpp \
		CDMARCXmlParserThread.h CDMARCXmlParserThread.cpp 
		
		
