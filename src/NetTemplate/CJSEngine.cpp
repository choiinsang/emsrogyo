#include "CJSEngine.h"

CJSEngine::CJSEngine(unsigned long rt_mem, unsigned long sc_mem)
: m_rt_mem(rt_mem), m_sc_mem(sc_mem)
{
	m_rt = JS_NewRuntime(rt_mem);
	m_ctx = JS_NewContext(m_rt, m_sc_mem);

	m_global = JS_NewObject(m_ctx, NULL, NULL, NULL);

        JS_InitStandardClasses(m_ctx, m_global);
}

CJSEngine::~CJSEngine()
{
	JS_DestroyContext(m_ctx);
	JS_DestroyRuntime(m_rt);
	JS_ShutDown();
}

bool CJSEngine::EvaluateScript(const char *script, const int length, jsval *rval)
{
	JSBool ok;

	ok = JS_EvaluateScript(m_ctx, m_global, script, length, NULL, 0, rval);
	
	return JSVAL_TO_BOOLEAN(ok);
}

