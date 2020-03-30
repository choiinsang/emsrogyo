INCLUDE=/usr/local/mysql/include
SP_INCLUDE=/usr/local/include/spidermonkey
BOOST_INCLUDE=../../boost_1_38_0
LIB=/usr/local/mysql/lib/mysql
SP_LIB=/usr/local/include/spidermonkey/Linux_All_DBG.OBJ/
LIBS=-lpthread -lmysqlclient
CFLAGS=-Wall -D__LINUX__

CMultiServerEpollTest2: CServerEpoll.h CMultiServerEpoll.h CMultiServerEpollTest2.cpp CClientEpoll.h CClientEpoll.cpp LockObject.h LockObject.cpp CRemoveList.h MyClient.h MyClient.cpp CMyRequest.h CMyRequest.cpp CMyThread.h CMyThread.cpp CMyServer.h CMyServer.cpp
	g++ -g $(CFLAGS) -I$(BOOST_INCLUDE) -I$(INCLUDE) -lpthread -o CMultiServerEpollTest2 CMultiServerEpollTest2.cpp CClientEpoll.cpp LockObject.cpp MyClient.cpp CMyRequest.cpp CMyThread.cpp CMyServer.cpp
CMultiServerEpollTest: CServerEpoll.h CMultiServerEpoll.h CMultiServerEpollTest.cpp CClientEpoll.h CClientEpoll.cpp LockObject.h LockObject.cpp CRemoveList.h MyClient.h MyClient.cpp CMyRequest.h CMyRequest.cpp CMyThread.h CMyThread.cpp CMyServer.h CMyServer.cpp
	g++ -g $(CFLAGS) -I$(BOOST_INCLUDE) -I$(INCLUDE) -lpthread -o CMultiServerEpollTest CMultiServerEpollTest.cpp CClientEpoll.cpp LockObject.cpp MyClient.cpp CMyRequest.cpp CMyThread.cpp CMyServer.cpp
CServerEpollTest: CServerEpoll.h CServerEpollTest.cpp CClientEpoll.h CClientEpoll.cpp LockObject.h LockObject.cpp CRemoveList.h 
	g++ -g $(CFLAGS) -I$(BOOST_INCLUDE) -I$(INCLUDE) -lpthread -o CServerEpollTest CServerEpollTest.cpp CClientEpoll.cpp LockObject.cpp  
CMultiServerTest2: CServer.h CMultiServer.h CMultiServerTest2.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp 
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CMultiServerTest2 CMultiServerTest2.cpp CClient.cpp LockObject.cpp
CServerTest: CServer.h CServerTest.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp 
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CServerTest CServerTest.cpp CClient.cpp LockObject.cpp 
CMultiServerTest: CServer.h CMultiServer.h CMultiServerTest.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp 
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CMultiServerTest CMultiServerTest.cpp CClient.cpp LockObject.cpp
CConnectTest: CServer.h CConnectTest.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp
	g++ $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CConnectTest CConnectTest.cpp CClient.cpp LockObject.cpp 
CServerTextTest: CServer.h CTextServer.h CServerTextTest.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp 
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CServerTextTest CServerTextTest.cpp CClient.cpp LockObject.cpp 
ttest7: ttest7.cpp CThread.h CThreadPool.h CMysql.h CMysql.cpp CServer.h CClient.h CClient.cpp LockObject.h LockObject.cpp CRemoveList.h
	g++ $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -L$(LIB) -o ttest7 ttest7.cpp CClient.cpp CMysql.cpp LockObject.cpp $(LIBS) 
ttest6: ttest6.cpp CThread.h CThreadPool.h CMysql.h CMysql.cpp
	g++ $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -L$(LIB) -o ttest6 ttest6.cpp CMysql.cpp $(LIBS) 
ZTest-cond: ZTest-cond.cpp CThread.h CThreadPool.h
	g++ -I$(INCLUDE) -L$(LIB) -o ZTest-cond ZTest-cond.cpp $(LIBS) 	
ttest2: ttest2.cpp CThread.h CThreadPool.h
	g++ -I$(INCLUDE) -L$(LIB) -o ttest2 ttest2.cpp $(LIBS) 	
ttest5: ttest5.cpp CThread.h CThreadPool.h 
	g++ $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -L$(LIB) -o ttest5 ttest5.cpp CClient.cpp $(LIBS) 
ttest4: ttest4.cpp CThread.h CThreadPool.h
	g++ $(CFLAGS) -I$(INCLUDE) -L$(LIB) -o ttest4 ttest4.cpp $(LIBS) 
ttest3: ttest3.cpp CThread.h CThreadPool.h
	g++ -I$(INCLUDE) -L$(LIB) -o ttest3 ttest3.cpp $(LIBS) 	
mysqltest: mysqltest.cpp CMysql.cpp CMysql.h
	g++ -I$(INCLUDE) -L$(LIB) -o mysqltest CMysql.cpp mysqltest.cpp $(LIBS) 
ms2: ms2.cpp CMysql.cpp CMysql.h
	g++ -I$(INCLUDE) -L$(LIB) -o ms2 CMysql.cpp ms2.cpp $(LIBS) 
stest: CServer.h CClient.h CClient.cpp stest.cpp
	g++ $(CFLAGS) -I$(INCLUDE) -L$(LIB) -o stest CClient.cpp stest.cpp $(LIBS) 
connect: connect.cpp
	g++ $(CFLAGS) -I$(INCLUDE) -L$(LIB) -o connect connect.cpp $(LIBS) 
clean:
	rm -f ~.o ttest3 ttest4 ttest5 ttest6 ttest7 mysqltest ms2 stest connect CServerTest CMultiServerTest CMultiServerTest2 CServerEpollTest CMultiServerEpollTest CMultiServerEpollTest2 JSEngineTest CServerUdpTest CHashTest
	
sm4: sm4.cpp CJSEngine.cpp CJSEngine.h JSTest.cpp JSTest.h 
	jsgen JSTest.h JSTestJS.h JSTestJS.cpp
	g++ -I$(INCLUDE) -I$(SP_INCLUDE) -L$(SP_LIB) -g -o sm4 sm4.cpp CJSEngine.cpp JSTest.cpp -ljs -DJSTHREAD_SAFE

JSEngineTest: CServer.h JSEngineTest.cpp CClient.h CClient.cpp LockObject.h LockObject.cpp CRemoveList.cpp CJSEngine.cpp CJSEngine.h 
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(SP_INCLUDE) -L$(SP_LIB) -lpthread -o JSEngineTest JSEngineTest.cpp CClient.cpp LockObject.cpp CRemoveList.cpp CJSEngine.cpp -ljs -DJSTHREAD_SAFE

CServerUdpTest: CServerUdp.h CServerUdpTest.cpp CClientUdp.h CClientUdp.cpp LockObject.h LockObject.cpp CSendPacket.h CSendPacket.cpp CSendQueue.h CSendQueue.cpp  
	g++ -g $(CFLAGS) -I$(INCLUDE) -I$(BOOST_INCLUDE) -lpthread -o CServerUdpTest CServerUdpTest.cpp CClientUdp.cpp LockObject.cpp CSendPacket.cpp CSendQueue.cpp
UdpClient: UdpClient.cpp
	g++ -o UdpClient -I$(INCLUDE) -I$(BOOST_INCLUDE) UdpClient.cpp
UdpClient2: UdpClient2.cpp
	g++ -o UdpClient2 -I$(INCLUDE) -I$(BOOST_INCLUDE) UdpClient2.cpp
CHashTest: CHashTest.cpp
	g++ -o CHashTest -I$(BOOST_INCLUDE) CHashTest.cpp
CResolverTest: CResolverTest.cpp CResolver.cpp
	g++ -o CResolverTest -I$(BOOST_INCLUDE) CResolverTest.cpp CResolver.cpp -lboost_system-gcc34-mt
