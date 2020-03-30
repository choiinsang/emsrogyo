#ifndef __CJSENGINE__
#define __CJSENGINE__

#define	XP_UNIX
#include "jscntxt.h"
#include "jsapi.h"

class CJSEngine
{
private:
	unsigned long m_rt_mem;
	unsigned long m_sc_mem;

	JSRuntime *m_rt;
	JSContext *m_ctx;
	JSObject *m_global;

public:
	CJSEngine(unsigned long rt_mem, unsigned long sc_mem);
	virtual ~CJSEngine();
	
	JSRuntime *getRuntime()	{ return m_rt; };
	JSContext *getContext()	{ return m_ctx; };
	JSObject *getGlobal()	{ return m_global; };

	bool EvaluateScript(const char *script, const int length, jsval *rval);
};

#endif

