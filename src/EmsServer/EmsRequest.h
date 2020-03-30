#ifndef __CEMS_REQUEST_HEADER__
#define __CEMS_REQUEST_HEADER__

#include "CRequest.h"
#include "EmsClient.h"


class CEmsRequest : public CRequest<CEmsClient>
{
	// Construction
	public:
		CEmsRequest() { };
		virtual ~CEmsRequest() { };
//	private:

};

#endif  //__CEMS_REQUEST_HEADER__

