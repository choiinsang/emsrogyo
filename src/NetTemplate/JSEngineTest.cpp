#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CTextServer.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClient.h"
#include "CJSEngine.h"
using namespace std;

#include <boost/shared_array.hpp>
using namespace boost;

// client

class MyClient : public CClient
{
public:
        MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList> pRemoveList);
        ~MyClient() {};
};

MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList> pRemoveList)
: CClient(ulKey, iMaxReadBuffer, iMaxWriteBuffer, pipe, pRemoveList)
{
//	printf("%ld\n", m_ulKey);
}

// server

typedef struct
{
	unsigned long TimeDiff;
	unsigned long Elapsed;
	string script;
} Timer;

typedef class CTextServer<MyClient> CVanilaServer;

#define gOutFile        stdout

class CMyServer : public CVanilaServer
{
private:
	shared_ptr<CJSEngine> m_pEngine;
public:
	map<unsigned long, shared_ptr<Timer> > m_Timers;

	CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	~CMyServer();
	void ParserMain(shared_ptr<MyClient> pClient, int nLength);
	void TimerFunc();

	void InitJSEngine();

// JavaScript Functions

	static JSBool Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);
	static JSBool SendJS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool SetTimer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool KillTimer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	static JSBool Exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	void ConsoleProcess(char *szTitle);
	bool ParseCmd(char *cmd);
};

shared_ptr<CMyServer> g_pServer;

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
	InitJSEngine();	
}

CMyServer::~CMyServer()
{
	
}

void CMyServer::InitJSEngine()
{
        m_pEngine = shared_ptr<CJSEngine>(new CJSEngine(1024*1024, 8192));
        JS_SetErrorReporter(m_pEngine->getContext(), ErrorReporter);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "print", Print, 0, 0);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "sendJS", SendJS, 0, 0);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "setTimer", SetTimer, 3, 0);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "killTimer", KillTimer, 1, 0);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "load", Load, 1, 0);
        JS_DefineFunction(m_pEngine->getContext(), m_pEngine->getGlobal(), "exec", Exec, 1, 0);
}

void CMyServer::ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
        fprintf(gOutFile, "%s at line number %d\n", message, report->lineno + 1);
}

JSBool CMyServer::Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i, n;
    JSString *str;

    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        fprintf(gOutFile, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    n++;
    if (n)
        fputc('\n', gOutFile);
    return JS_TRUE;

}

JSBool CMyServer::Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if( argc != 1 ) return JS_FALSE;
	JSString *file;
	
	file = JS_ValueToString(cx, argv[0]);
	char *fn = JS_GetStringBytes(file);

	FILE *fp = fopen(fn, "rt");
	if( NULL == fp ) return JS_FALSE;
	unsigned long length;

	fseek(fp, 0, SEEK_END);
	length = ftell(fp);

	shared_array<char> contents = shared_array<char>(new char [length + 1]);
	memset(contents.get(), 0, length + 1);
	fseek(fp, 0, SEEK_SET);
	if( 1 != fread(contents.get(), length, 1, fp) ) return JS_FALSE;
	fclose(fp);

	jsval ok;
        g_pServer->m_pEngine->EvaluateScript(contents.get(), length, &ok);
	
	return JS_TRUE;
}

JSBool CMyServer::Exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if( argc != 1 ) return JS_FALSE;
	JSString *str;
	
	str = JS_ValueToString(cx, argv[0]);
	char *cmd = JS_GetStringBytes(str);
	if( 0 != system(cmd) ) 
		return JS_FALSE;
	else
		return JS_TRUE;
}

void CMyServer::TimerFunc()
{
	map<unsigned long, shared_ptr<Timer> >::iterator I;
	shared_ptr<Timer> pTimer;

	for( I = m_Timers.begin(); I != m_Timers.end(); I++ )
	{
		pTimer = (shared_ptr<Timer>)((*I).second);
		pTimer->Elapsed += m_timer;
		if( pTimer->Elapsed > pTimer->TimeDiff )
		{
			jsval rval;
        		m_pEngine->EvaluateScript(pTimer->script.c_str(), pTimer->script.size(), &rval);
			pTimer->Elapsed = 0;
		}
	}
}

void CMyServer::ParserMain(shared_ptr<MyClient> pClient, int nLength)
{
	shared_array<char> msg = shared_array<char>(new char[nLength + 1]);

	memset((char *)msg.get(), 0, nLength + 1);
	memcpy((void *)msg.get(), (void *)pClient->m_ReadBuffer.get(), nLength);
	
	printf("Client %ld said : %s\n", pClient->m_ulKey, (const char *)msg.get());
	fflush(stdout);

	jsval args[2];
	jsval rval;

	args[0] = INT_TO_JSVAL(pClient->m_ulKey);
	args[1] = STRING_TO_JSVAL(JS_NewString(m_pEngine->getContext(), msg.get(), nLength));	

	JS_CallFunctionName(m_pEngine->getContext(), m_pEngine->getGlobal(), "parserJS", 2, args, &rval);
}

void CMyServer::ConsoleProcess(char *szTitle)
{
        char cmd[1024];
        int carrot;

        printf("%s console system activated.\n\n", szTitle);

        memset(cmd, 0, 1024);
        carrot = 0;

        while( true )
        {
                char strCmd[1024];
                memset(strCmd, 0, 1024);
                printf("# ");
                fgets(strCmd, 1024, stdin);

                if( !ParseCmd(strCmd) ) break;
        }
        printf("\n\n%s console system deactivated.\n", szTitle);
}

bool CMyServer::ParseCmd(char *cmd)
{
	jsval rval;

	if( 0 == strcmp("allout\n", cmd) ) return false;
        if( 0 == strcmp("quit\n", cmd) ) return false;

        m_pEngine->EvaluateScript(cmd, strlen(cmd), &rval);

        return true;
}

JSBool CMyServer::SendJS(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if( argc != 2 ) return JS_FALSE;
	int32 lKey;
	
	JS_ValueToInt32(cx, argv[0], &lKey);

        map <unsigned int, shared_ptr< MyClient > >::iterator R;

	R = g_pServer->m_ClientList.find(lKey);
	if( R == g_pServer->m_ClientList.end() ) return JS_FALSE;

	shared_ptr<MyClient> pClient = (*R).second;
	
	JSString *str = JS_ValueToString(cx, argv[1]);	
	char *msg = JS_GetStringBytes(str);
	if( pClient->Send(msg, strlen(msg)) )
		return JS_TRUE;
	else
		return JS_FALSE;
}

JSBool CMyServer::SetTimer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if( argc != 3 ) return JS_FALSE;
	int32 id, TimeDiff;
	JSString *str;

	JS_ValueToInt32(cx, argv[0], &id);
	JS_ValueToInt32(cx, argv[1], &TimeDiff);
	str = JS_ValueToString(cx, argv[2]);

	map<unsigned long, shared_ptr<Timer> >::iterator I;
	I = g_pServer->m_Timers.find(id);
	if( g_pServer->m_Timers.end() != I ) g_pServer->m_Timers.erase(I);

	shared_ptr< Timer > pTimer = shared_ptr< Timer >(new Timer);

	pTimer->TimeDiff = TimeDiff;
	pTimer->Elapsed = 0; 
	pTimer->script = (string)(JS_GetStringBytes(str));

	g_pServer->m_Timers.insert(make_pair( id, pTimer) );

	jsval ok;
	char cmd[1024];
	memset(cmd, 0, 1024);
	sprintf(cmd, "print('Timer %d added interval %ld');", id, pTimer->TimeDiff);
	
        g_pServer->m_pEngine->EvaluateScript(cmd, strlen(cmd), &ok);
	
	return JS_TRUE;
}

JSBool CMyServer::KillTimer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if( argc != 1 ) return JS_FALSE;
	int32 id;
	JS_ValueToInt32(cx, argv[0], &id);

	map<unsigned long, shared_ptr<Timer> >::iterator I;
	I = g_pServer->m_Timers.find(id);
	if( g_pServer->m_Timers.end() == I ) return JS_FALSE;

	g_pServer->m_Timers.erase(I);
	
	jsval ok;
	char cmd[1024];
	memset(cmd, 0, 1024);
	sprintf(cmd, "print('Timer %d killed');", id);
	
        g_pServer->m_pEngine->EvaluateScript(cmd, strlen(cmd), &ok);
	return JS_TRUE;
}

int main(int argc, char* argv[])
{
	g_pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 100, 5000, 3000, NULL));

	g_pServer->InitNetwork();
	g_pServer->Start();

	g_pServer->ConsoleProcess("JSEngineTest");

	g_pServer->Stop();
	g_pServer->CleanUpNetwork();

	return 0;
}
