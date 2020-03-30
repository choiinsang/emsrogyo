#ifndef __CSEND_QUEUE__
#define __CSEND_QUEUE__

#include "LockObject.h"
#include "CSendPacket.h"

#include <string>
#include <queue>
using namespace std;

class CSendQueue : public LockObject
{
private:
	queue< shared_ptr<CSendPacket> > m_queue;

public:
	CSendQueue();
	~CSendQueue();
	
	void AddQueue(string IP, int Port, char *data, int nSize);
	void AddQueue(shared_ptr<CSendPacket> pPacket);
	shared_ptr<CSendPacket> Pop(); 
	int GetSize();
};

#endif
