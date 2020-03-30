#ifndef __JSTEST__
#define	XP_UNIX
#include "jsapi.h"

class JSTest
{
public:
	/* javascript constructors */
	JSTest();
	/* end */
	virtual ~JSTest();
	
	/* javascript functions */
	int add(int a, int b, int c);
	/* end */

	#include "JSTestJS.h"
};

#endif
