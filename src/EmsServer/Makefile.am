
bin_PROGRAMS = IEmsServer IEmsAgent reQManager


IEmsServer_CPPFLAGS = -Wall -O -g -D__LINUX__ -D_DAEMON_VERSION_=0  -D_USE_DOMAINLIST_=1 -D_Bool=bool -I/usr/local/include  -I../boost_1_41_0  -I./amqpcpp-master/include -I../NetTemplate  -I../Libgabia/inc -I/usr/include/opendkim 
IEmsServer_LDFLAGS  = -L../Libgabia/lib -lgabia -lm -lpthread -lresolv  -lbind -lrabbitmq -L./amqpcpp-master/lib -lamqpcpp -lopendkim 
IEmsServer_SOURCES  = EmsMain.cpp \
		amqpcpp-master/include/AMQPcpp.h \
		amqpcpp-master/src/AMQP.cpp amqpcpp-master/src/AMQPBase.cpp \
		amqpcpp-master/src/AMQPException.cpp amqpcpp-master/src/AMQPMessage.cpp \
		amqpcpp-master/src/AMQPExchange.cpp amqpcpp-master/src/AMQPQueue.cpp \
		../Common/INIFile.h ../Common/INIFile.cpp  \
		../NetTemplate/packet_header.h \
		../NetTemplate/CClientList.h  \
		../NetTemplate/CRemoveList.h  \
		../NetTemplate/NetTemplateDefine.h  \
		../NetTemplate/LockObject.h ../NetTemplate/LockObject.cpp \
		../NetTemplate/Log.h  ../NetTemplate/Log.cpp \
		../NetTemplate/CClientEpoll.h  ../NetTemplate/CClientEpoll.cpp \
		../NetTemplate/CServerEpoll.h \
		../NetTemplate/CMultiServerEpoll.h \
		../Libgabia/src/sth_syslog.h ../Libgabia/src/sth_syslog.c\
		../NetTemplate/CThread.h  \
		../NetTemplate/CRequest.h  \
		FuncString.h FuncString.cpp \
		EmsCommon.h EmsDefine.h EmsDefine.cpp\
		EmsConfig.h EmsConfig.cpp \
		EmsDKIM.h EmsDKIM.cpp \
		EmsQueue.h EmsQueue.cpp \
		EmsRequest.h  \
		EmsThread.h EmsThread.cpp \
		EmsClient.h EmsClient.cpp \
		EmsRMQConnect.h EmsRMQConnect.cpp \
		EmsRMQManager.h EmsRMQManager.cpp \
		EmsLog.h EmsLog.cpp \
		EmsWorkerThreadPool.h EmsWorkerThreadPool.cpp \
		EmsWorkServer.h EmsWorkServer.cpp \
		EmsWorkerManager.h EmsWorkerManager.cpp \
		EmsMX.h EmsMX.cpp \
		EmsMXManager.h EmsMXManager.cpp \
		EmsDomainListSet.h EmsDomainListSet.cpp 
		
		
		

IEmsAgent_CPPFLAGS = -Wall -O -g -D__LINUX__ -D_DAEMON_VERSION_=0 -D_USE_DOMAINLIST_=0  -I/usr/local/include -I/usr/local/include/libgabia  -I../boost_1_41_0  -I./amqpcpp-master/include -I../NetTemplate -I../Libgabia/inc  -I/usr/include/opendkim
IEmsAgent_LDFLAGS  = -L../Libgabia/lib -lgabia -lm -lpthread  -liconv -L/usr/lib64/mysql -lmysqlclient -lrabbitmq -L./amqpcpp-master/lib -lamqpcpp 
 
IEmsAgent_SOURCES = EmsAgentMain.cpp \
		./amqpcpp-master/include/AMQPcpp.h \
		./amqpcpp-master/src/AMQP.cpp amqpcpp-master/src/AMQPBase.cpp \
		./amqpcpp-master/src/AMQPException.cpp amqpcpp-master/src/AMQPMessage.cpp \
		./amqpcpp-master/src/AMQPExchange.cpp amqpcpp-master/src/AMQPQueue.cpp \
		../Common/INIFile.h ../Common/INIFile.cpp  \
		../NetTemplate/DefineNetGlobal.h \
		../NetTemplate/packet_header.h \
		../NetTemplate/CClientList.h  \
		../NetTemplate/CRemoveList.h  \
		../NetTemplate/NetTemplateDefine.h  \
		../NetTemplate/LockObJect.h ../NetTemplate/LockObject.cpp \
		../NetTemplate/Log.h  ../NetTemplate/Log.cpp \
		../NetTemplate/CClientEpoll.h  ../NetTemplate/CClientEpoll.cpp \
		../NetTemplate/CServerEpoll.h \
		../Libgabia/inc/libgabia.h  \
		../Libgabia/inc/sth_misc.h ../Libgabia/src/sth_misc.c \
		../Libgabia/inc/sth_syslog.h ../Libgabia/src/sth_syslog.c \
		../NetTemplate/CThread.h  \
		../NetTemplate/CThreadPool.h  \
		../NetTemplate/CRequest.h  \
		../NetTemplate/CMysql.h  ../NetTemplate/CMysql.cpp \
		FuncString.h FuncString.cpp \
		EmsCommon.h EmsDefine.h EmsDefine.cpp \
		EmsDefineString.h EmsStructDefine.h \
		EmsConfig.h EmsConfig.cpp \
		EmsClient.h EmsClient.cpp \
		EmsQueue.h EmsQueue.cpp \
		EmsRMQConnect.h EmsRMQConnect.cpp \
		EmsRMQManager.h EmsRMQManager.cpp \
		EmsLog.h EmsLog.cpp \
		EmsMysql.h  EmsMysql.cpp \
		EmsDbThread.h  EmsDbThread.cpp  \
		EmsDbCommonQThread.h  EmsDbCommonQThread.cpp \
		EmsAgentServer.h EmsAgentServer.cpp \
		EmsQProcThread.h EmsQProcThread.cpp 
		

reQManager_CPPFLAGS = -Wall -O -g -D__LINUX__ -D_DAEMON_VERSION_=0  -D_USE_DOMAINLIST_=1 -D_Bool=bool -I../Libgabia/inc -I/usr/local/include  -I../boost_1_41_0  -I./amqpcpp-master/include -I../NetTemplate  
reQManager_LDFLAGS  = -L../Libgabia/lib -lgabia -lm -lpthread -lrabbitmq -L./amqpcpp-master/lib -lamqpcpp
reQManager_SOURCES  = reQManagerMain.cpp \
		amqpcpp-master/include/AMQPcpp.h \
		amqpcpp-master/src/AMQP.cpp amqpcpp-master/src/AMQPBase.cpp \
		amqpcpp-master/src/AMQPException.cpp amqpcpp-master/src/AMQPMessage.cpp \
		amqpcpp-master/src/AMQPExchange.cpp amqpcpp-master/src/AMQPQueue.cpp \
		../Common/INIFile.h ../Common/INIFile.cpp  \
		../NetTemplate/packet_header.h \
		../NetTemplate/CClientList.h  \
		../NetTemplate/CRemoveList.h  \
		../NetTemplate/NetTemplateDefine.h  \
		../NetTemplate/LockObject.h ../NetTemplate/LockObject.cpp \
		../NetTemplate/Log.h  ../NetTemplate/Log.cpp \
		../NetTemplate/CClientEpoll.h  ../NetTemplate/CClientEpoll.cpp \
		../NetTemplate/CServerEpoll.h \
		../NetTemplate/CMultiServerEpoll.h \
		../Libgabia/src/sth_syslog.h ../Libgabia/src/sth_syslog.c\
		../NetTemplate/CThread.h  \
		../NetTemplate/CRequest.h  \
		FuncString.h FuncString.cpp \
		EmsCommon.h EmsDefine.h EmsDefine.cpp\
		EmsConfig.h EmsConfig.cpp \
		EmsQueue.h EmsQueue.cpp \
		EmsRMQConnect.h EmsRMQConnect.cpp \
		EmsRMQManager.h EmsRMQManager.cpp \
		EmsLog.h EmsLog.cpp\
		manual.h \
		reQManagerProc.h reQManagerProc.cpp

